<?php

/**
 * @file certificate.api.php
 * Document certificate hooks.
 */

/**
 * Implementation of hook_access_certificate().
 *
 * Any module wishing to award a certificate based on arbitrary criteria should
 * implement this hook. The $node is the node in question, the $user is the user
 * viewing the node.
 *
 * @return mixed
 *   FALSE if user should not be shown the menu tab or link.
 *   TRUE if user should be able to download a certificate.
 *   A string if the user should be displayed a friendly message instead.
 */
function hook_access_certificate($node, $user) {
  if ($user->score > $node->pass_rate) {
    // Let the user get a certificate if they passed something.
    return TRUE;
  }
}

/**
 * Implementation of certificate_template_id_alter().
 *
 * Arbitrarily override the template ID that will be loaded when the user
 * downloads a certificate.
 */
function hook_certificate_template_id_alter(&$template_id, $node, $user) {
  if ($node->nid % 2 == 0) {
    // Set certificate to use node 476 when the node NID is even.
    $template_id = 476;
  }
}
