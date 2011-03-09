<?php

/**
 * @file
 * Administrative pages for the module.
 */

/**
 * =================== Certificates template-mapping form (used by certificate_settings_form() & node form alter) ===================
 */

/**
 * Returns the form for the per-node certificate settings.
 *
 * This is shared by the settings page and the node edit page.
 *
 * @param $node
 *   The fully loaded node object if we've got it.
 *
 * @return
 *   The form array for the per-node certificate settings.
 *
 */
function certificate_type_mapping_form($form_state, $node = NULL) {
  // Get existing node-specific template settings.
  $node_template_settings = $node->certificate['node_settings'];

  // Get existing templates.
  $templates = certificate_certificate_load_all();
  $template_options[0] = 'Global';
  foreach ($templates as $key => $template) {
    $template_options[$key] = $template['title'];
  }

  if (variable_get('certificate_field_grouping',0)) {
    // Group stuff
    $fieldgroups = variable_get('certificate_field_groups', array());

    // Generate form elements based on selected field's allowed values.
      if (count($fieldgroups)) {
        foreach ($fieldgroups as $key => $fieldgroup) {
          // We don't care about $fieldgroup - just the key.
          // Get node-based setting, if not set, use global.
          if ($node) {
            $template = $node_template_settings[$key];
            $template = $template ? $template : 0;
          }
          else {
            $template = variable_get("certificate_type_{$key}_template",0);
          }

          $form[$key] = array(
            '#type' => 'select',
            '#title' => $key,
            '#options' => $template_options,
            '#default_value' => $template,
          );
        }
      }
      else {
        $form['nogroups'] = array(
          '#prefix' => '<p>',
          '#suffix' => '</p>',
          '#value' => t("Please go to !groups to configure certificate field mapping groups.", array('!groups' =>  l('groups', 'admin/settings/certificate/mapping/groups'))),
        );
      }
  }
  else {
    // Get field's options from types selection.
    $field_options_array = certificate_get_selected_type_options();
    if (arg(0) == 'node' && !count($field_options_array)) {
      $form['#type'] = 'markup';
      $form['#value'] = "<div>" . t("Please !link before using certificates.", array('!link' => l('set up field mappings', 'admin/settings/certificate/mapping'))) . "</div>";
    }

    // Generate form elements based on selected field's allowed values.
    if ($field_options_array) {
      foreach ($field_options_array as $key => $option_name) {
        // Get node-based setting, if not set, use global.
        if ($node) {
          $template = $node_template_settings[$key];
          $template = $template ? $template : 0;
        }
        else {
          $template = variable_get("certificate_type_{$key}_template",0);
        }

        $form[$key] = array(
          '#type' => 'select',
          '#title' => $option_name,
          '#options' => $template_options,
          '#default_value' => $template,
        );
      }
    }
  }

  return $form;
}

/**
 * Page to show certificate types.
 */
function certificate_sets_page(&$certificate_type = array()) {
  drupal_set_title('Criteria');
  $sql = 'select ct.type_id, ct.name, node.title from {certificate_types} ct
  left join {node} node on node.nid = template_id';
  $query = db_query($sql);
  while ($row = db_fetch_object($query)) {
    $cells = (array)$row;
    $cells[] = _certificate_certificate_type_links($cells);
    $rows[] = $cells;
  }
  return $out . theme('table', array(
    'Type ID',
    'Type Name',
    'Template Name',
    'Operations',
  ), $rows);
}

/**
 * Save/Update a certificate type.
 */
function certificate_sets_form_submit($form, &$form_state) {
  $action = 'saved';
  if ($form_state['values']['type_id']) {
    $update = 'type_id';
    $action = 'updated';
  }
  $write = $form_state['values'];
  drupal_write_record('certificate_types', $write, $update);
  drupal_set_message(t("Certificate type $action."));
  drupal_goto('admin/settings/certificate/sets');
}

/**
 * Page to show certificate type criteria.
*/
function certificate_sets_checks_page($type) {
  $out = '';
  $out .= "<h3>Editing criteria for <em>\"{$type->name}\"</em></h3>";
  $out .= "<p>The <em>\"{$type->title}\"</em> template will show when all of the criteria are met.</p>";

  $sql = 'select * from {certificate_criteria}
    where type_id = %d';
  $query = db_query($sql, $type->type_id);

  while ($row = db_fetch_array($query)) {
    $row['check_key'] = unserialize($row['check_key']);
    $row['check_key'] = $row['check_key'][1];
    $row['operations'] = certificate_sets_checks_links($row);
    $rows[] = $row;
  }

  $out .= theme('table', array(
    'Type ID','Criteria ID','Check Type','Check Key','Check Value','Operations'
  ), $rows);
  $out .= drupal_get_form('certificate_sets_checks_form', $type);

  return $out;
}

function certificate_sets_checks_form_submit($form, $form_state) {
  $write = $form_state['values'];
  $write['check_key'] = serialize($write['check_key']);

  if (drupal_write_record('certificate_criteria', $write)) {
    drupal_set_message('Criteria added.');
  } else {
    drupal_set_message('Criteria failed.');
  }

  drupal_goto('admin/settings/certificate/sets/' . $form_state['values']['type_id'] .'/criteria');
}

/**
 * Form for certificate type criteria.
 */
function certificate_sets_checks_form($form_state, $type = array()) {
  $form = array();

  $form['table_start'] = array(
    '#type' => 'markup',
    '#value' => '<table>',
  );

  $checks = 0;

  for($i = 1; $i <= $checks+1; $i++) {
    $form['row_start'] = array(
      '#type' => 'markup',
      '#value' => '<tr>',
    );

    $form['check_type'] = array(
    '#title' => 'Type',
    '#description' => 'Select the criteria type.',
      '#type' => 'select',
      '#options' => array('node' => 'node','group' => 'group', 'profile' => 'profile'),
      '#prefix' => '<td>',
      '#suffix' => '</td>',
    );

    module_load_include('inc','hs_field_selector','hs_field_selector');
    $form['check_key'] = hs_field_selector_form_element();
    $form['check_key']['#title'] = 'Criteria';
    $form['check_key']['#config']['resizable'] = 0;
    $form['check_key']['#prefix'] = '<td>';
    $form['check_key']['#suffx'] = '</td>';

    $form['check_value'] = array(
      '#title' => 'Value',
      '#type' => 'textfield',
      '#size' => 20,
      '#prefix' => '<td>',
      '#suffix' => '</td>',
    );

    $form['type_id'] = array(
      '#type' => 'hidden',
      '#value' => $type->type_id,
    );

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Add',
      '#prefix' => '<td><br/>',
      '#suffix' => '</td>',
    );

    $form['row_end'] = array(
      '#type' => 'markup',
      '#value' => '</tr>',
    );
  }

  $form['table_end'] = array(
    '#type' => 'markup',
    '#value' => '</table>',
  );

  return $form;
}

/**
 * Form for certificate type.
 */
function certificate_sets_form(&$form_state, $type = array()) {
  $form = array();

  if (count($type)) {
    drupal_set_title('Edit criteria set');
  }
  else {
    drupal_set_title('Create criteria set');
  }

  // Get existing templates.
  $templates = certificate_certificate_load_all();
  $template_options = array();
  foreach ($templates as $key => $template) {
    $template_options[$key] = $template['title'];
  }

  module_load_include('inc', 'hs_field_selector');

  $form['type_id'] = array(
    '#type' => 'hidden',
    '#default_value' => $type->type_id,
  );

  $form['name'] = array(
    '#title' => 'Name',
    '#type' => 'textfield',
    '#size' => 20,
    '#prefix' => '<td>',
    '#suffix' => '</td>',
    '#default_value' => $type->name,
    '#description' => t('A name that will not be displayed to users'),
  );

  $form['template_id'] = array(
    '#type' => 'select',
    '#title' => 'Template',
    '#options' => $template_options,
    '#prefix' => '<td>',
    '#suffix' => '</td>',
    '#default_value' => $type->template_id,
    '#description' => t('Choose the certificate that will be rendered for this criteria.'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#title' => 'Save',
    '#value' => 'Save',
  );

  return $form;
}

/**
 * =================== Certificates alter node form (using template-mapping form) ===================
 */

/**
 * Alters the node form to inject the appropriate per-node template settings.
 */
function certificate_alter_node_form(&$form, &$form_state) {
  global $user;

  // Node is not saved but previewed (nid is empty).
  if (isset($form['#node']->build_mode) && $form['#node']->build_mode == NODE_BUILD_PREVIEW) {
    $node = $form['#node'];
  }
  // Editing a node.
  elseif (!empty($form['#node'])) {
    $node = $form['#node'];
  }

  if (module_exists('og')) {
    $group = og_get_group_context();
    $group_admin = og_is_group_admin($group);
  }

  // Add template-mapping form.
  // Note, make sure to set #tree to TRUE.
  if (user_access('administer certificates') || $group_admin) {
    $form['certificate'] = array(
      '#type' => 'fieldset',
      '#title' => t('Certificate template settings'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#weight' => 100,
      '#tree' => TRUE,
      '#group' => TRUE,
    );

    // Do we get the form state on submit?
    #drupal_set_message('<pre>'. print_r($node_template_settings, TRUE) .' </pre>');

    #module_load_include('inc', 'certificate', 'certificate.admin');
    $form['certificate']['node_settings'] = certificate_type_mapping_form($form_state, $node);
  }
}

/**
 * =================== Certificates settings form @ admin/settings/certificate/mapping ===================
 */

/**
 * Form for field mapping allowed values to certificate templates.
 */
function certificate_settings_form(&$form_state) {
  drupal_set_title('Certificate field mapping');
  $form['certificate_settings']['#tree'] = TRUE;

  // Certificate types.
  $types = variable_get('certificate_field_certificate_type', '');
  $form['certificate_settings']['types'] = array(
    '#type' => 'fieldset',
    '#title' => t('Mapped profile field'),
    '#collapsible' => TRUE,
    '#collapsed' => $types != '',
  );

  if (module_exists('content')) {
    $field_options = certificate_fields();
    $form['certificate_settings']['types']['certificate_field_certificate_type'] = array(
      '#type' => 'select',
      '#options' => $field_options,
      '#default_value' => variable_get('certificate_field_certificate_type', 0),
      '#description' => t("Select which field should be used to generate certificate types. <b>WARNING:</b> changing this may cause undesired effects."),
    );
  }
  else {
    $form['certificate_settings']['no_cck_message'] = array(
      '#value' => variable_get('no_cck_message', t('You must enable the <a href="!cck_url">cck module</a> to use this feature.', array('!cck_url' => url('http://drupal.org/project/cck')))),
    );
  }

  // Set course defaults.
  // Add template-mapping form.
  module_load_include('inc', 'certificate', 'certificate.admin');

  $form['certificate_settings']['map'] = certificate_type_mapping_form(array());

  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

/**
 * Submit handler for global field mapping form.
 *
 * @TODO: If there is an older selected field with more options, delete them from variables (count($field_options_array)).
 */
function certificate_settings_form_submit($form, &$form_state) {
  // Delete any old variables, for example if there was an older selected field with more options.
  $variables = db_query("SELECT name FROM {variable} WHERE name LIKE '%s_%%'", 'certificate_type');
  while ($variable = db_fetch_object($variables)) {
      variable_del($variable->name);
  }

  // Get the field key.
  $settings = $form_state['values']['certificate_settings'];
  $selected = reset($form_state['values']['certificate_settings']['types']);

  $field_options = certificate_fields();

  // Set type variable.
  variable_set('certificate_field_certificate_type', $selected);

  // Get field's options from types selection.
  if (variable_get('certificate_field_grouping',0)) {
    $field_options_array = variable_get('certificate_field_groups', array());
  }
  else {
    $field_options_array = certificate_get_selected_type_options();
  }

  // Set variables for mappings.
  if ($field_options_array) {
    foreach ($field_options_array as $key => $option_name) {
      variable_set('certificate_type_'. $key .'_template', $settings['map'][$key]);
    }
  }

  // Friendly message.
  drupal_set_message(t('The certificate mapping options have been saved.'));
}

/**
 * Build the operations for a certificate type.
 */
function _certificate_certificate_type_links($type) {
  $path = drupal_get_path('module', 'certificate') . '/images/';

  $links['edit'] = array(
    'title' => t('Edit set'),
    'href' => 'admin/settings/certificate/sets/' . $type['type_id'] . '/edit',
    'html' => TRUE,
    'query' => drupal_get_destination(),
  );

  $links['criteria'] = array(
    'title' => t('Edit criteria'),
    'href' => 'admin/settings/certificate/sets/' . $type['type_id'] . '/criteria',
    'html' => TRUE,
  );

  $links['delete'] = array(
    'title' => t('Delete'),
    'href' => 'admin/settings/certificate/sets/' . $type['type_id'] . '/delete',
    'html' => TRUE,
    'query' => drupal_get_destination(),
  );

  return theme('links', $links);
}

/**
 * Build the operations for a certificate type criterion.
 */
function certificate_sets_checks_links($criteria) {
  $path = drupal_get_path('module', 'certificate') . '/images/';

  $links['delete'] = array(
    'title' => t('Delete'),
    'href' => 'admin/settings/certificate/criteria/' . $criteria['check_id'] . '/delete',
    'html' => TRUE,
    'query' => drupal_get_destination(),
  );

  return theme('links',$links);
}

/**
 * Theme overview form.
 *
 * Arranges certificates in a table.
 *
 * @ingroup themeable
 * @ingroup forms
 * @see certificate_overview_form()
 */
function theme_certificate_overview_form($form) {
  $rows = array();
  foreach (element_children($form['certificates']) as $key) {
    $row = array();

    // Render the hidden 'certificate id' field and the title of the certificate into the
    // same column of the row.
    $row[] = drupal_render($form['certificates'][$key]['cid']) . drupal_render($form['certificates'][$key]['title']);

    // Render the edit and delete links into their own column.
    $row[] = drupal_render($form['certificates'][$key]['operations']);

    // Add the new row to our collection of rows.
    $rows[] = array(
      'data' => $row,
    );
  }

  // If there were no certificates found, let users know.
  if (count($rows) == 0) {
    $rows[] = array(array('data' => t('No certificate templates have been added.'), 'colspan' => 3));
  }

  // Render a list of header titles, and our array of rows, into a table. Even
  // we've already rendered all of our certificates, we always call drupal_render()
  // on the form itself after we're done, so hidden security fields and other
  // elements (like buttons) will appear properly at the bottom of the form.
  $header = array(t('Title'), t('Operations'));
  // Add an optional caption that appears above the table.
  $caption = t('');
  $output = theme('table', $header, $rows, array('id' => 'certificates-overview'), $caption);
  $output .= drupal_render($form);

  return $output;
}

/**
 * =================== Certificates User certicates history form @ admin/settings/certificate/history ===================
 */

/**
 * Build a form to record the history of all user's certificates.
 */
function certificate_history_form(&$form_state) {
  $form['test'] = array(
    '#value' => t(''),
  );

  return $form;
}

/**
 * Theme the form as a table.
 */
function theme_certificate_history_form($form) {
  $rows = array();
  $header = array(t('User'), t('Course'), t('Operations'));
  $rows = array(
    // Simple row
    array( 'Fred Flintstone', 'Course 101', 'View | Edit | Email'),
    array( 'Fred Flintstone', 'Course 102', 'View | Edit | Email'),
    array( 'Joan Smith', 'Course 102', 'View | Edit | Email'),
  );

  $caption = t('');
  $output = theme('table', $header, $rows, array('id' => 'certificates-history'), $caption);
  $output .= drupal_render($form);

  return $output;
}

// Criterion delete.

function certificate_criterion_delete_form_submit(&$form, &$form_state) {
  $sql = "delete from {certificate_criteria} where check_id = %d";
  db_query($sql, $form_state['values']['check_id']);
}

function certificate_criterion_delete_form(&$form_state, $criterion) {
  $form = array();
  $form['check_id'] = array(
    '#value' => $criterion->check_id,
    '#type' => 'hidden',
  );
  $description = t('This criterion looks for %check_type objects, and matches %check_key against "%check_value"', array(
    '%check_type' => $criterion->check_type,
    '%check_key' => $criterion->check_key[0] . '-' . $criterion->check_key[1],
    '%check_value' => $criterion->check_value,
  ));
  $form = confirm_form($form, "Delete criterion for defined <em>{$criterion->name}</em> certificate type?", '', $description, 'Delete', 'Cancel', 'delete-criterion');
  return $form;
}

// Type delete.

function certificate_set_delete_form_submit(&$form, &$form_state) {
  $sql = "delete from {certificate_types} where type_id = %d";
  db_query($sql, $form_state['values']['type_id']);
  $sql = "delete from {certificate_criteria} where type_id = %d";
  db_query($sql, $form_state['values']['type_id']);
}

function certificate_set_delete_form(&$form_state, $type) {
  $form = array();
  $form['type_id'] = array(
    '#value' => $type->type_id,
    '#type' => 'hidden',
  );
  $description = "This will also delete the associated criteria.";
  $form = confirm_form($form, "Delete <em>{$type->name}</em> certificate template?", '', $description, 'Delete', 'Cancel', 'delete-type');
  return $form;
}

/** Grouping */

function certificate_field_grouping_form() {
  $form = array();

  $form['certificate_field_grouping'] = array(
    '#type' => 'checkbox',
    '#title' => t('Group fields?'),
    '#default_value' => variable_get('certificate_field_grouping', 0),
    '#description' => t('For when there are too many field options to manage individually, group fields to match certificates globally and on certifiable nodes.'),
  );

  if (variable_get('certificate_field_grouping', 0) == 1) {
    $fieldgroups = variable_get('certificate_field_groups', array());

    if (count($fieldgroups)) {
      // Fieldset tree for field groups.
      $form['certificate_field_groups'] = array(
        '#title' => 'Field groups',
        '#type' => 'fieldset',
        '#tree' => TRUE,
      );
    }
    else {
      $form['nogroups'] = array(
        '#prefix' => '<p>',
        '#suffix' => '</p>',
        '#value' => t("No mapped field groups configured."),
      );
    }

    $form['certificate_add_new_group'] = array(
      '#prefix' => '<div>',
      '#suffix' => '</div>',
      '#type' => 'markup',
      '#value' => l('Add a new group?', 'admin/settings/certificate/mapping/groups/add'),
    );

    // Create multiselect box for each field key.
    foreach ($fieldgroups as $key => $fieldgroup) {
      $form['certificate_field_groups'][$key] = array(
        '#title' => $key,
        '#type' => 'select',
        '#multiple' => TRUE,
        '#options' => certificate_get_selected_type_options(),
        '#suffix' => l('Delete ' . $key, "admin/settings/certificate/mapping/groups/delete/$key"),
        '#default_value' => $fieldgroup,
      );
    }
  }

  $form['submit'] = array(
    '#value' => 'Submit',
    '#type' => 'submit',
  );

  return $form;
}

function certificate_field_grouping_form_submit(&$form, &$form_state) {
  variable_set('certificate_field_grouping', $form_state['values']['certificate_field_grouping']);

  if (is_array($form_state['values']['certificate_field_groups'])) {
    variable_set('certificate_field_groups', $form_state['values']['certificate_field_groups']);
  }

  drupal_set_message('Grouping settings updated.');
}

function certificate_field_grouping_page() {
  drupal_set_title('Certificate field group mapping');
  return drupal_get_form('certificate_field_grouping_form');
}

function certificate_field_grouping_add_form() {
  $form = array();

  $form['fieldgroup_name'] = array(
    '#title' => 'Name of field group',
    '#type' => 'textfield',
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Create',
  );

  return $form;
}

function certificate_field_grouping_add_form_validate(&$form, &$form_state) {
  $fieldgroup_name = $form_state['values']['fieldgroup_name'];
  $fieldgroups = variable_get('certificate_field_groups', array());

  if (in_array($fieldgroup_name, array_keys($fieldgroups))) {
    form_set_error('fieldgroup_name', 'Duplicate, please choose another name.');
  }
}

function certificate_field_grouping_add_form_submit(&$form, &$form_state) {
  $fieldgroup_name = $form_state['values']['fieldgroup_name'];
  $fieldgroups = variable_get('certificate_field_groups', array());

  $fieldgroups[$fieldgroup_name] = array();
  variable_set('certificate_field_groups', $fieldgroups);
  drupal_set_message(t('Fieldgroup %f created.', array('%f' => $fieldgroup_name)));
  drupal_goto('admin/settings/certificate/mapping/groups');
}

function certificate_field_grouping_add_page() {
  return drupal_get_form('certificate_field_grouping_add_form');
}

function certificate_field_grouping_delete_form_submit(&$form, &$form_state) {
  $fieldgroup_name = $form_state['values']['fieldgroup_name'];
  $fieldgroups = variable_get('certificate_field_groups', array());

  unset($fieldgroups[$fieldgroup_name]);

  variable_set('certificate_field_groups', $fieldgroups);
  drupal_set_message(t('Fieldgroup %f deleted.', array('%f' => $fieldgroup_name)));
  drupal_goto('admin/settings/certificate/mapping/groups');
}

function certificate_field_grouping_delete_form($form, $key) {
  $form = array();

  $form['fieldgroup_name'] = array(
    '#type' => 'hidden',
    '#value' => $key,
    '#access' => FALSE,
  );

  return confirm_form($form, "Delete field group $key?", 'admin/settings/certificate/mapping/groups', 'This action cannot be undone. Access to matched certificates will be lost.', 'Delete', 'Cancel');
}

function certificate_field_grouping_delete_page($key) {
  return drupal_get_form('certificate_field_grouping_delete_form', $key);
}

function certificate_templates_list() {
  $sql = "select * from {node} where type='certificate'";
  $result = db_query($sql);
  while ($row = db_fetch_array($result)) {
    $destination = drupal_get_destination();
    $certificates[] = array(
                           'title'=>$row['title'],
                           'edit'=>l(t('edit'), "node/{$row['nid']}/edit/{$destination}", array('attributes' => array('class' => 'edit-link'))),
                           'delete'=>l(t('delete'), "node/{$row['nid']}/delete/{$destination}", array('attributes' => array('class' => 'delete-link'))),
                           'preview'=> l(t('PDF'), "admin/settings/certificate/templates/preview/{$row['nid']}", array('attributes' => array('class' => 'preview-link'))),
                          );
  }

  return theme_table(array(t('Title'),t('Edit'),t('Delete'),t('Preview')), $certificates);
}

function certificate_admin_settings_form() {
  $form = array();

  $form['certifiable_types'] = array(
  '#type'=> 'fieldset',
  '#title' => 'Certifiable node types',
  );

  foreach (node_get_types() as $type => $info) {
    $form['certifiable_types']["certificate_certifiable_$type"] = array(
      '#title' => $info->name,
      '#type' => 'checkbox',
      '#default_value' => variable_get("certificate_certifiable_$type", 0),
    );
  }

  $form['certificate_snapshots'] = array(
    '#title' => 'Use certificate snapshots?',
    '#description' => 'Certificates will only be generated once per node/user.',
    '#type' => 'checkbox',
    '#default_value' => variable_get('certificate_snapshots', 0),
  );

  return system_settings_form($form);
}
