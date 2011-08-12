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
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_CONFIGURATION,
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
  $items['toolbox-server'] = array(
    'description' => 'Start a simple server from the command-line with your Drupal root as the document root.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_CONFIGURATION,
    'aliases' => array('server'),
    'arguments' => array(
      'action' => 'Signal to start, restart or stop the apache daemon.  Defaults to start.  (start|restart|stop)',
    ),
    'options' => array(
      'host' => 'Specify the host to use as the ServerName for apache.  If not specified, drush will attempt to retrieve the hostname from the uri context (set with the --uri option), and finally fall back to using "localhost" as the default.',
      'port' => 'Specify the port for apache to listen on.  If not specified, drush will attempt to retrieve the port from the uri context (set with the --uri option), and finally fall back to using 3000 as the default.  Using a port lower than 1024 will require running this command as root.',
      'conf-dir' => 'The directory to store the configuration and log files necessary for running apache.  If not specified, drush will create a directory named ".drush-server" in the site root.',
    ),
    'examples' => array(
      'drush server' => 'Start a server using default options.',
      'drush @local server' => 'Start a server using a drush site alias.  This allows you to start the server without first changing to the appropriate directory or specifying a root and/or uri parameter for drush.',
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
  $command = $ctags .' --langmap=php:.engine.inc.module.theme.php.test.install --php-kinds=cdfi --languages=php --recurse';
  $status = drush_shell_exec($command);
  if (!$status) {
    return drush_set_error('DRUSH_TOOLBOX_CTAGS_ERROR', dt('Error running ctags'));
  }

  $root = drush_get_context('DRUSH_DRUPAL_ROOT');
  drush_log(dt('Tag file created at !file.', array('!file' => "$root/tags")), 'ok');
}

/**
 * Validation callback for `drush toolbox-server`
 */
function drush_toolbox_server_validate($arg = 'start') {
  if (!in_array($arg, array('start', 'stop', 'restart', 'graceful'))) {
    return drush_set_error('DRUSH_TOOLBOX_SERVER_INVALID_ARG', dt('Invalid argument.'));
  }

  // Use a hidden .drush-server directory if conf-dir is not specified.
  $conf_dir = drush_get_option('conf-dir', drush_get_context('DRUSH_DRUPAL_ROOT') .'/.drush-server');
  if (!drush_mkdir($conf_dir)) {
    return drush_set_error('DRUSH_SERVER_NO_CONF_DIR', dt('Unable to create temporary directory !dir.', array('dir' => $conf_dir)));
  }
  drush_set_option('conf-dir', $conf_dir);

  $uri = drush_get_context('DRUSH_URI');

  // The host gets set by the first of these conditions.
  // 1. Via the --host parameter.
  // 2. The host specified in the DRUSH_URI context.
  // 3. 'localhost' if all else fails.
  $host = drush_get_option('host', parse_url($uri, PHP_URL_HOST));
  empty($host) && ($host = 'localhost');
  drush_set_option('host', $host);

  // The port gets set by the first of these conditions.
  // 1. Via the --port parameter.
  // 2. The port specified in the DRUSH_URI context.
  // 3. 3000 if all else fails.
  // TODO: if port < 1024, ensure that user has is root or sudo.
  $port = drush_get_option('port', parse_url($uri, PHP_URL_PORT));
  empty($port) && ($port = 3000);
  drush_set_option('port', $port);

  $log_dir = drush_get_option(array('log-dir'), "$conf_dir/log");
  // Ensure the log_dir exists.
  if (!drush_mkdir($log_dir)) {
    return drush_set_error('DRUSH_SERVER_NO_LOG_DIR', dt('Unable to create log directory !dir.', array('dir' => $log_dir)));
  }
  drush_set_option('log-dir', $log_dir);

  drush_set_context('DRUSH_TOOLBOX_SERVER_CONF_FILE', "$conf_dir/httpd.conf");
}

/**
 * Command callback for `drush toolbox-server`
 *
 * Start server in the Drupal root with the version specific start script.
 */
function drush_toolbox_server($arg = 'start') {
  if ($arg == 'start') {
    drush_print(dt('Starting server...'));
  }

  $root = drush_get_context('DRUSH_DRUPAL_ROOT');
  $conf_file = _drush_server_create_conf_file();

  $command = join(' ', array(
    'apachectl',
    "-f \"{$conf_file}\"",
    "-k \"$arg\"",
  ));

  $status = drush_shell_exec($command);
  if (!$status) {
    return drush_set_error('DRUSH_TOOLBOX_SERVER_UNKNOWN_ERROR', dt('Unknown error.'));
  }

  $output = drush_shell_exec_output();
  if (!empty($output)) {
    foreach ($output as $line) {
      // TODO: parse the output from apachectl to get better messages.
      drush_log($line, 'warning');
    }
  }

  $conf = _drush_server_conf();
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
 * Consolidate all server options and settings into one array.
 */
function _drush_server_conf() {
  $conf['conf_file'] = drush_get_context('DRUSH_TOOLBOX_SERVER_CONF_FILE');
  $conf['doc_root'] = drush_get_context('DRUSH_DRUPAL_ROOT');;
  $conf['conf_dir'] = drush_get_option('conf-dir');
  $conf['log_dir'] = drush_get_option('log-dir');
  $conf['host'] = drush_get_option('host');
  $conf['port'] = drush_get_option('port');

  return $conf;
}

/**
 * Build the temporary httpd conf file for this docroot.
 */
function _drush_server_create_conf_file() {
  $command = drush_get_command();
  $template = $command['path'] .'/includes/httpd_osx.conf';

  $conf = _drush_server_conf();

  $directives[] = "#";
  $directives[] = "# Base configuration from $conf[conf_file]";
  $directives[] = "#";
  $directives[] = file_get_contents($template);

  $directives[] = "\n\n";

  $directives[] = "#";
  $directives[] = "# Custom configuration built by drush";
  $directives[] = "#";
  $directives[] = "PidFile $conf[conf_dir]/httpd.pid";
  $directives[] = "LockFile $conf[conf_dir]/accept.lock";
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
