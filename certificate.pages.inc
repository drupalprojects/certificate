<?php

/**
 * @file
 * Builds the module's user-facing pages.
 *
 * Contains a simple page callback and themable function for rendering a single
 * certificate.
 */

/**
 * Get certificate for a specific node.
 */
function certificate_node_certificate($node) {
  global $user;

  if ($user->uid) {
    $account = $user;
  }

  $perm = certificate_can_access_certificate($node, $user);

  if ($perm != 1) {
    //$perm is a string! show error.
    return $perm;
  }

  // Use account of a different user if allowed.
  if (user_access('administer certificates') && arg(3) > 0) {
    $account = user_load(arg(3));
  }

  return certificate_single($account, $node);
}

/**
 * Generate a single certificate.
 *
 * Check to see if user already has a certificate for a course. If so, serve it from the DB. If not, generate one and save it to the DB.
 */
function certificate_single($account, $node) {
  global $user;
  $output = '';

  // See if if user already has a certificate for a course.
  $snapshot = certificate_snapshot_load($account, $node);

  // Dev preview.
  $preview = isset($_GET['preview']) && user_access('administer certificates');

  // Group support.
  if ($node->og_groups) {
    $group = node_load(key($node->og_groups));
  }

  // If so, grab from the DB. If not, generate a new one.
  if ($snapshot && variable_get('certificate_snapshots', 0)) {
    $output = $snapshot['snapshot'];
  }
  else {
    // Do matches on fieldmapping
    // Find certificate based on set profile field. This is lowest priority
    // Find certificate based on node setting. This is highest priority
    $profile = _certificate_get_user_profile($account);

    // Get existing node-specific template settings.
    $node_template_settings = $node->certificate['node_settings'];

    // Get certificate 'type' field.
    $certificate_type_field = certificate_get_selected_type();

    if (variable_get('certificate_field_grouping', 0)) {
      // Get user's 'type' from $account.
      $user_types = $profile->{$certificate_type_field};
      $field_options_array = variable_get('certificate_field_groups', array());
      foreach ($field_options_array as $key => $fieldgroup) {
        foreach ($fieldgroup as $value) {
          foreach ($user_types as $user_type) {
            $user_type = $user_type['value'];
            if ($user_type == $value) {
              // This node has settings saved in the DB - use the per-node settings?
              if ($node_template_settings[$key]) {
                $template_id = $node_template_settings[$key];
              }
              // Else this node doesn't have settings saved in the DB yet - use the global settings?
              else {
                $template_id = variable_get('certificate_type_' . $key . '_template', null);
              }
              // Found a match, do not continue.
              break 3;
            }
          }
        }
      }
    }
    else {
      // Get user's 'type' from $account.
      $user_type = $profile->{$certificate_type_field}[0]['value'];
      // Get certificate template that matches type.
      // Get field's options from types selection.
      // This returns groups if set to grouping.
      $field_options_array = certificate_get_selected_type_options();

      // Get form element names based on selected field's options (if field is selected).
      if (is_array($field_options_array) && $account->uid != 0) {
        foreach ($field_options_array as $key => $option_name) {
          if ($key == $user_type) {
            // This node has settings saved in the DB - use the per-node settings?
            if ($node_template_settings[$key]) {
              $template_id = $node_template_settings[$key];
            }
            // Else this node doesn't have settings saved in the DB yet - use the global settings?
            else {
              $template_id = variable_get('certificate_type_' . $key . '_template', null);
            }
          }
        }
      }
    }

    // If no local mappings, try matching with criteria.
    if (!$template_id) {
      $sql = "select * from {certificate_criteria} cc
      left join {certificate_types} ct on ct.type_id = cc.type_id
      order by cc.type_id";
      $query = db_query($sql);
      $certificate_types = array();
      while ($criteria = db_fetch_object($query)) {
        $criteria->check_key = unserialize($criteria->check_key);
        $criteria->check_key = $criteria->check_key[1];
        $certificate_types[$criteria->type_id][$criteria->check_id] = $criteria;
      }
      foreach ($certificate_types as $certificate_type) {
        $use_this_type = true;
        foreach ($certificate_type as $criteria) {
          // Checking a field value: $criteria->check_type with key $criteria->check_key against $criteria->check_value.
          if (empty($criteria->check_key)) {
            // If no field set, check the node type.
            if (!(${$criteria->check_type}->type == $criteria->check_value)) {
              $use_this_type = false;
            }
          }
          else if (!(${$criteria->check_type}->{$criteria->check_key}[0]['value'] == $criteria->check_value)) {
            $use_this_type = false;
          }
        }
        if ($use_this_type) {
          $template_id = $criteria->template_id;
          break;
        }
      }
    }

    // No mappings at all. Pass to other modules.
    if (!$template_id) {
      $modules = module_invoke_all('certificate_map_options');
      foreach ($modules as $map_type => $map) {
        $mapping = variable_get("certificate_map_{$map_type}", array());
        $mapping = array_filter($mapping);
        $matches = module_invoke_all('certificate_map', $node, $user, $map_type, $mapping, array());
        if (count($matches)) {
          $template_id = $mapping[$matches[0]]; // First one for now.
        }
      }
    }

    drupal_alter('certificate_template_id', $template_id, $node, $account);

    $template = node_load($template_id);

    if ($template && $node) {
      // Prepend output with UTF 8 meta tag.
      // See http://code.google.com/p/wkhtmltopdf/issues/detail?id=556
      // Print module gets around this because the meta tag in Drupal is already
      // set - but here, we have to add it since we are only delivering the node
      // body.
      $output = '<meta charset="utf-8">' . theme('certificate_certificate', $account, $profile, $node, $group, $template);

      // Now save this generated certificate as a 'snapshot' in the database.
      $snapshot['uid'] = $account->uid;
      $snapshot['nid'] = $node->nid;
      $date = getdate();
      $snapshot['date'] = $date[0];
      $snapshot['snapshot'] = $output;
      if (!$preview) {
        certificate_snapshot_save($snapshot);
      }
    }
    else {
      $no_match_string = t('Sorry, there is no certificate available based on your profile data.');
      // Don't show Drupal frame if no certificate matches.
      global $baseurl;
      $ref = parse_url($_SERVER['HTTP_REFERER']);
      $ref['path'] = str_replace($baseurl, '', $ref['path']);
      if (preg_match('#^/moodle#', $ref['path'])) {
        print $no_match_string;
        exit;
      }
      else {
        return $no_match_string;
      }
    }
  }

  $print_pdf_pdf_tool = variable_get('print_pdf_pdf_tool', '');
  if ($print_pdf_pdf_tool == '') {
    if ($user->uid == 1) {
      drupal_set_message('Certificate cannot be displayed because you have not selected a PDF generation tool in ' . l('Printer, e-mail and PDF versions', 'admin/settings/print/pdf') . '.', 'error');
    }
    else {
      drupal_set_message('PDF generation tool is not configured.');
    }
    return '';
  }

  module_load_include('pages.inc', 'print_pdf', 'print_pdf');

  if ($preview) {
    print $output;
  }
  else {
    // Rewrite image URLs using Print.
    $pattern = '!<(img\s[^>]*?)>!is';
    $output = preg_replace_callback($pattern, '_print_rewrite_urls', $output);
    global $conf;
    $conf['print_pdf_page_orientation'] = $template->certificate['orientation'];

    if (basename($print_pdf_pdf_tool) == 'dompdf_config.inc.php') {
      $print_func = '_print_pdf_dompdf';
    }
    elseif (basename($print_pdf_pdf_tool) == 'tcpdf.php') {
      $print_func = '_print_pdf_tcpdf';
    }
    elseif (substr(basename($print_pdf_pdf_tool, '.exe'), 0, 11) == 'wkhtmltopdf') {
      $print_func = '_print_pdf_wkhtmltopdf';
    }

    $print_func(array('node' => $node), $output, $node->title . '.pdf');
  }
}

/**
 * Preview certificate.
 *
 * @todo This code is copied from above, needs to be refactored to avoid
 * copypasta.
 */
function certificate_preview($template_id) {
  $template = node_load($template_id);
  if ($template) {
    $output = theme('certificate_certificate', $account, $profile, $node, $group, $template);
  }

  $print_pdf_pdf_tool = variable_get('print_pdf_pdf_tool', '');
  if ($print_pdf_pdf_tool == '') {
    if ($user->uid == 1) {
      drupal_set_message('Certificate cannot be displayed because you have not selected a PDF generation tool in ' . l('Printer, e-mail and PDF versions', 'admin/settings/print/pdf') . '.', 'error');
    }
    else {
      drupal_set_message('PDF generation tool is not configured.');
    }
    return '';
  }

  module_load_include('pages.inc', 'print_pdf', 'print_pdf');

  // Rewrite image URLs using Print.
  $pattern = '!<(img\s[^>]*?)>!is';
  $output = preg_replace_callback($pattern, '_print_rewrite_urls', $output);
  global $conf;
  $conf['print_pdf_page_orientation'] = $template->certificate['orientation'];

  if (basename($print_pdf_pdf_tool) == 'dompdf_config.inc.php') {
    $print_func = '_print_pdf_dompdf';
  }
  elseif (basename($print_pdf_pdf_tool) == 'tcpdf.php') {
    $print_func = '_print_pdf_tcpdf';
  }
  elseif (substr(basename($print_pdf_pdf_tool, '.exe'), 0, 11) == 'wkhtmltopdf') {
    $print_func = '_print_pdf_wkhtmltopdf';
  }

  $print_func(array('node' => $template), $output, $template->title . '.pdf');
}

/**
 * Theme a single certificate.
 *
 * Does token replace (new style [], and old style %)
 *
 * @param $account
 *   The user account being viewed.
 *
 * @ingroup themeable
 *
 * @return The certificate HTML with all tokens translated.
 */
function theme_certificate_certificate($account, $profile, $node, $group, $template) {
  $types = array(
    'global' => null,
    'node' => $node,
    'user' => $account,
    'certificate' => array(
      'node' => $node,
      'user' => $account,
      'group' => $group,
      'profile' => $profile
    ),
  );

  // Invoke hook_certificate_body_alter() to allow all modules to alter the template body.
  drupal_alter('certificate_body', $template->body, $account, $node);

  if (module_exists('token')) {
    if (module_exists('purl')) {
      purl_disable(TRUE);
    }
    $template->body = token_replace_multiple($template->body, $types);
  }

  $template->body = check_markup($template->body, $template->format, FALSE);

  return $template->body;
}
