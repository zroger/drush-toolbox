<?php
/**
* @file
* This is a modified version of index.php, to be used with phpsh.
*/

/**
* Root directory of Drupal installation.
*/
define('DRUPAL_ROOT', getcwd());

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';

drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
