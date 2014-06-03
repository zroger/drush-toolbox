<?php
/**
* @file
* This is a modified version of index.php, to be used with phpsh.
*/

/**
 * Root directory of Drupal installation.
 */

require_once getcwd() . '/core/vendor/autoload.php';
require_once getcwd() . '/core/includes/bootstrap.inc';

drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
