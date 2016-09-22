<?php

/**
 * Custom Entity admin
 */
class CertificateEntityUIController extends EntityDefaultUIController {

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

}
