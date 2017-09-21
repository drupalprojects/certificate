<?php

/**
 * Custom Entity admin
 */
class CertificateEntityUIController extends EntityBundleableUIController {

  public $overviewPagerLimit = 1000;

  protected function overviewTableRow($conditions, $id, $entity, $additional_cols = array()) {
    $rows = parent::overviewTableRow($conditions, $id, $entity, $additional_cols);
    $rows[] = l(t('PDF'), $this->path . '/preview/' . $id);
    return $rows;
  }

  protected function operationCount() {
    return parent::operationCount() + 1;
  }

  /**
   * Override the table that comes from EntityDefaultUIController so we can sort
   * it by label.
   */
  public function overviewTable($conditions = array()) {
    $table = parent::overviewTable($conditions);
    uasort($table['#rows'], array($this, 'labelSorter'));
    return $table;
  }

  /**
   * uasort() callback to sort by label of a table row.
   */
  public function labelSorter($a, $b) {
    if (!isset($b[0]['data']['#label'])) {
      return -1;
    }
    if (!isset($a[0]['data']['#label'])) {
      return 1;
    }
    return strcasecmp($a[0]['data']['#label'], $b[0]['data']['#label']);
  }

  function hook_menu() {
    $items = parent::hook_menu();

    // Set this on the object so classes that extend hook_menu() can use it.
    $this->id_count = count(explode('/', $this->path));
    $plural_label = isset($this->entityInfo['plural label']) ? $this->entityInfo['plural label'] : $this->entityInfo['label'] . 's';


    $items[$this->path] = array(
      'title' => $plural_label,
      'page callback' => 'drupal_get_form',
      'page arguments' => array($this->entityType . '_overview_form', $this->entityType),
      'description' => 'Manage ' . $plural_label . '.',
      'access callback' => 'entity_access',
      'access arguments' => array('view', $this->entityType),
      'file' => 'includes/entity.ui.inc',
    );
    $items[$this->path . '/list'] = array(
      'title' => 'List',
      'type' => MENU_DEFAULT_LOCAL_TASK,
      'weight' => -10,
    );
    return $items;
  }

}
