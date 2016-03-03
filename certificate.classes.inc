<?php

/**
 * Custom Entity admin
 */
class CertificateEntityUIController extends EntityDefaultUIController {

  protected function overviewTableRow($conditions, $id, $entity, $additional_cols = array()) {
    $rows = parent::overviewTableRow($conditions, $id, $entity, $additional_cols);
    $rows[] = l(t('PDF'), $this->path . '/preview/' . $id);
    return $rows;
  }

  protected function operationCount() {
    return parent::operationCount() + 1;
  }

}
