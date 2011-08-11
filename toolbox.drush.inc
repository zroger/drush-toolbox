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
  );
  $items['toolbox-ctags'] = array(
    'description' => 'Generate a "tags" file using exuberant ctags. Requires exuberant ctags.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_CONFIGURATION,
    'aliases' => array('ctags'),
  );
  $items['toolbox-server'] = array(
    'description' => 'Start a simple server from the command-line with your Drupal root as the document root.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_CONFIGURATION,
    'aliases' => array('server'),
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
  $_SERVER['HTTP_HOST'] = drush_get_context('DRUSH_URI');

  $root = drush_get_context('DRUSH_DRUPAL_ROOT');
  $phpsh = drush_get_option('phpsh');

  $command = join(' ', array(
    $phpsh,
    drush_escapeshellarg($include),
  ));

  if (drush_get_context('DRUSH_VERBOSE')) {
    drush_print('Executing: ' . $command);
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
  $command = $ctags .' --langmap=php:.engine.inc.module.theme.php.test.install --php-kinds=cdfi --languages=php --recurse';
  $status = drush_shell_exec($command);
  if (!$status) {
    return drush_set_error('DRUSH_TOOLBOX_CTAGS_ERROR', dt('Error running ctags'));
  }

  $root = drush_get_context('DRUSH_DRUPAL_ROOT');
  drush_log(dt('Tag file created at !file.', array('!file' => "$root/tags")), 'ok');
}

/**
 * Command callback for `drush toolbox-server`
 *
 * Start server in the Drupal root with the version specific start script.
 */
function drush_toolbox_server($arg = 'start') {
  if (!in_array($arg, array('start', 'stop', 'restart', 'graceful'))) {
    return drush_set_error('DRUSH_TOOLBOX_SERVER_INVALID_ARG', dt('Invalid argument.'));
  }

  if ($arg == 'start') {
    drush_print(dt('Starting server...'));
  }

  $root = drush_get_context('DRUSH_DRUPAL_ROOT');
  $conf = _drush_server_conf($root);
  _drush_server_create_conf_file($root);

  $command = join(' ', array(
    'apachectl',
    "-f \"{$conf['conf_file']}\"",
    "-k \"$arg\"",
  ));

  $status = drush_shell_exec($command);
  if (!$status) {
    return drush_set_error('DRUSH_TOOLBOX_SERVER_UNKNOWN_ERROR', dt('Unknown error.'));
  }

  $output = drush_shell_exec_output();
  if (!empty($output)) {
    foreach ($output as $line) {
      drush_log($line, 'warning');
    }
  }

  switch ($arg) {
    case 'start':
      drush_log(dt('Server started on !host:!port.', array('!host' => $conf['host'], '!port' => $conf['port'])), 'ok');
      if (drush_get_context('DRUSH_VERBOSE')) {
        drush_print(dt('    Config file:           !conf_file', array('!conf_file' => $conf['conf_file'])));
        drush_print(dt('    Error log:             !log_dir/error_log', array('!log_dir' => $conf['log_dir'])));
        drush_print(dt('    Access log:            !log_dir/access_log', array('!log_dir' => $conf['log_dir'])));
      }

      break;
    case 'stop':
      drush_log(dt('Server stopped on !host:!port.', array('!host' => $conf['host'], '!port' => $conf['port'])), 'ok');
      break;
    case 'restart':
    case 'graceful':
      drush_log(dt('Server restarted on !host:!port.', array('!host' => $conf['host'], '!port' => $conf['port'])), 'ok');
      break;
  }
}

/**
 * Build an array of config variables needed to build the temporary httpd conf
 * file.
 */
function _drush_server_conf($root) {
  static $conf = array();

  if (empty($conf)) {
    $conf['tmp_dir'] = drush_get_option(array('tmp-dir'), FALSE);
    if (empty($conf['tmp_dir'])) {
      // Try to use the alias if available.
      $alias_context = drush_get_context('alias');
      if (!empty($alias_context['name'])) {
        $conf['tmp_dir'] = drush_find_tmp() .'/'. str_replace('@', '', $alias_context['name']);
      }
      // Last resort.  Let drush create a temp dir.
      else {
        $conf['tmp_dir'] = drush_tempdir();
      }
    }
    $conf['host'] = drush_get_option(array('host', 'h'), 'localhost');
    $conf['port'] = drush_get_option(array('port', 'p'), 3000);
    $conf['log_dir'] = drush_get_option(array('log-dir'), $conf['tmp_dir'] .'/log');

    $conf['doc_root'] = $root;
    $conf['conf_file'] = $conf['tmp_dir'] .'/httpd.conf';

    if (!drush_mkdir($conf['tmp_dir'])) {
      return drush_set_error('DRUSH_SERVER_NO_TMP_DIR', dt('Unable to create temporary directory !dir.', array('dir' => $conf['tmp_dir'])));
    }

    if (!drush_mkdir($conf['log_dir'])) {
      return drush_set_error('DRUSH_SERVER_NO_LOG_DIR', dt('Unable to create log directory !dir.', array('dir' => $conf['log_dir'])));
    }
  }

  return $conf;
}

/**
 * Build the temporary httpd conf file for this docroot.
 */
function _drush_server_create_conf_file($root) {
  $conf = _drush_server_conf($root);

  $commands = drush_get_commands();
  $template = $commands['server']['path'] .'/includes/httpd_osx.conf';

  $directives[] = "#";
  $directives[] = "# Base configuration from $conf[conf_file]";
  $directives[] = "#";
  $directives[] = file_get_contents($template);

  $directives[] = "\n\n";

  $directives[] = "#";
  $directives[] = "# Custom configuration built by drush";
  $directives[] = "#";
  $directives[] = "PidFile $conf[tmp_dir]/httpd.pid";
  $directives[] = "LockFile $conf[tmp_dir]/accept.lock";
  $directives[] = '';
  $directives[] = "ServerName $conf[host]";
  $directives[] = "Listen $conf[port]";
  $directives[] = '';
  $directives[] = "ErrorLog $conf[log_dir]/error_log";
  $directives[] = 'LogFormat "%h %l %u %t \"%r\" %>s %b" common';
  $directives[] = "CustomLog $conf[log_dir]/access_log common";
  $directives[] = '';
  $directives[] = "DocumentRoot \"$conf[doc_root]\"";
  $directives[] = "<Directory \"$conf[doc_root]\">";
  $directives[] = "  AllowOverride all";
  $directives[] = "</Directory>";

  $content = join("\n", $directives);
  file_put_contents($conf['conf_file'], $content);

  return $conf['conf_file'];
}
