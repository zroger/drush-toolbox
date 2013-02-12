<?php
/**
 * @file Drush Developers' Toolbox commands
 */

/**
 * Implementation of hook_drush_help().
 */
function toolbox_drush_help($section) {
  switch ($section) {
    case 'meta:toolbox:title':
      return dt("Developers' Toolbox commands");
    case 'meta:toolbox:summary':
      return dt('Tools for working with Drupal in a development environment.');
  }
}

/**
 * Implementation of hook_drush_command().
 */
function toolbox_drush_command() {
  $items['toolbox-console'] = array(
    'description' => 'Run an interactive shell in the current Drupal environment. Requires a phpsh (http://phpsh.org/).',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_CONFIGURATION,
    'aliases' => array('console'),
    'options' => array(
      'phpsh' => 'Specify the path to the phpsh executable.  If not provided, `which phpsh` will be used.',
    ),
    'examples' => array(
      'drush console' => 'Start a console using default options.  Assumes a configured Drupal root can be located by drush and the phpsh executable can be located with `which phpsh`.',
      'drush @local console' => 'Start a console using a drush site alias.  This allows you to start a console without first changing to the appropriate directory or specifying a root and/or uri parameter for drush.',
      'drush console --phpsh=/usr/local/bin/phpsh' => 'Specify where to find phpsh, in case it is not in your $PATH variable, or not the first one found.',
    ),
  );
  $items['toolbox-ctags'] = array(
    'description' => 'Generate a "tags" file using exuberant ctags. Requires exuberant ctags.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'aliases' => array('ctags'),
    'options' => array(
      'ctags' => 'Specify the path to the ctags executable.  If not provided, `which ctags` will be used.',
    ),
    'examples' => array(
      'drush ctags' => 'Create a tags file in the current drupal root.',
      'drush @local ctags' => 'Specify a drush site alias to use.  The tags file will be created in the root specified by the alias.',
      'drush ctags --ctags=/usr/local/bin/ctags' => 'Specify where to find the ctags executable, in case it is not in your $PATH variable, or not the first one found.',
    ),
  );
  return $items;
}

/**
 * Validation callback for `drush toolbox-console`
 */
function drush_toolbox_console_validate() {
  $phpsh = drush_get_option('phpsh');

  if (empty($phpsh)) {
    $status = drush_shell_exec("which phpsh 2>&1");
    if ($status === FALSE) {
      return drush_set_error('DRUSH_TOOLBOX_PHPSH_NOT_FOUND', dt('Cannot find phpsh executable.'));
    }
    $output = drush_shell_exec_output();
    $phpsh = $output[0];
  }

  if (!file_exists($phpsh)) {
    return drush_set_error('DRUSH_TOOLBOX_PHPSH_NOT_FOUND', dt('Cannot find phpsh executable.'));
  }

  if (!is_executable($phpsh)) {
    return drush_set_error('DRUSH_TOOLBOX_PHPSH_NOT_EXECUTABLE', dt('The specified phpsh file is not executable.'));
  }

  drush_set_option('phpsh', $phpsh);
}

/**
 * Command callback for `drush toolbox-console`
 *
 * Start phpsh in the Drupal root with the version specific startup script.
 */
function drush_toolbox_console() {
  $command = drush_get_command();
  $include = $command['path'] .'/includes/console_'. drush_drupal_major_version() .'.inc';

  // Add some required items to the $_SERVER array, which will be used as the
  // startup environment for phpsh.
  $_SERVER['REMOTE_ADDR'] = '127.0.0.1';
  $_SERVER['SCRIPT_NAME'] = '/index.php';
  $_SERVER['REQUEST_URI'] = '/';
  $_SERVER['HTTP_HOST'] = parse_url(drush_get_context('DRUSH_URI'), PHP_URL_HOST);

  $root = drush_get_context('DRUSH_DRUPAL_ROOT');
  $phpsh = drush_get_option('phpsh');

  $command = join(' ', array(
    $phpsh,
    drush_escapeshellarg($include),
  ));

  if (drush_get_context('DRUSH_VERBOSE')) {
    drush_print('Executing: ' . $command);
    drush_print_r($_SERVER);
  }

  proc_close(proc_open($command, array(0 => STDIN, 1 => STDOUT, 2 => STDERR), $pipes, $root, $_SERVER));
}

/**
 * Validation callback for `drush toolbox-ctags`
 */
function drush_toolbox_ctags_validate() {
  $ctags = drush_get_option('ctags');

  if (empty($ctags)) {
    $status = drush_shell_exec("which ctags 2>&1");
    if ($status === FALSE) {
      return drush_set_error('DRUSH_TOOLBOX_CTAGS_NOT_FOUND', dt('Cannot find ctags executable.'));
    }
    $output = drush_shell_exec_output();
    $ctags = $output[0];
  }
  else if (!file_exists($ctags)) {
    return drush_set_error('DRUSH_TOOLBOX_CTAGS_NOT_FOUND', dt('Cannot find ctags executable.'));
  }

  if (!is_executable($ctags)) {
    return drush_set_error('DRUSH_TOOLBOX_CTAGS_NOT_EXECUTABLE', dt('The specified ctags file is not executable.'));
  }

  $status = drush_shell_exec("$ctags --version 2>&1");
  if ($status === FALSE) {
    return drush_set_error('DRUSH_TOOLBOX_CTAGS_INVALID', dt('Incompatible version of ctags.  Please install exuberant ctags.'));
  }

  $output = drush_shell_exec_output();
  if (!preg_match('/[Ee]xuberant/', $output[0])) {
    return drush_set_error('DRUSH_TOOLBOX_CTAGS_INVALID', dt('Incompatible version of ctags.  Please install exuberant ctags.'));
  }

  drush_set_option('ctags', $ctags);
}

/**
 * Command callback for `drush toolbox-ctags`
 *
 * Generate ctags for Drupal.  Requires exuberant ctags.
 * See http://openflows.com/blog/mvc/2009/01/27/using-exuberant-ctags-drupal
 */
function drush_toolbox_ctags() {
  $ctags = drush_get_option('ctags');
  $command = $ctags .' --langmap=php:.engine.inc.module.theme.php.install.test.profile --php-kinds=cdfi --languages=php --recurse --exclude="\.git" --exclude="\.svn" --exclude="\.hg" --exclude="\.bzr" --exclude="CVS" --totals=yes --tag-relative=yes --regex-PHP="/abstract\s+class\s+([^ ]+)/\1/c/" --regex-PHP="/interface\s+([^ ]+)/\1/c/" --regex-PHP="/(public\s+|static\s+|abstract\s+|protected\s+|private\s+)function\s+\&?\s*([^ (]+)/\2/f/"';
  $status = drush_shell_exec($command);
  if (!$status) {
    return drush_set_error('DRUSH_TOOLBOX_CTAGS_ERROR', dt('Error running ctags'));
  }

  drush_log(dt('Tag file created'), 'ok');
}
