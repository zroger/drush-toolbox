<?php
/**
 * Feather is a light-weight, minimal configuration script for running an Apache
 * http server in a development environment.
 */

/**
 * Implementation of hook_drush_help().
 */
function feather_drush_help($section) {
  switch ($section) {
    case 'meta:feather:title':
      return dt("Feather HTTP server commands");
    case 'meta:feather:summary':
      return dt('Start and stop the server, add and remove virtual hosts.');
  }
}

/**
 * Implementation of hook_drush_command().
 */
function feather_drush_command() {
  $items['feather'] = array(
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  $items['feather-start'] = array(
    'description' => 'Start the web server.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['feather-stop'] = array(
    'description' => 'Stop the web server.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['feather-status'] = array(
    'description' => 'Display the current status (running/not running) of the server.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['feather-check-config'] = array(
    'description' => 'Check the syntax of the generated configuration files.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['feather-error-log'] = array(
    'description' => 'Display the error log using `tail`.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  $items['feather-list'] = array(
    'description' => 'Display all configured virtual hosts.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['feather-add'] = array(
    'description' => 'Add a virtual host configuration.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['feather-remove'] = array(
    'description' => 'Remove a virtual host configuration.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  return $items;
}

/**
 * Include a file from the feather includes directory.
 */
function feather_include($filename) {
  include_once dirname(__FILE__) .'/includes/'. $filename;
}

/**
 * Command callback for `drush feather`.
 */
function drush_feather() {
  $args = func_get_args();
  $command = array_shift($args);
  if (empty($command)) {
    $command = 'status';
  }

  $commands = drush_get_commands();
  $command = $commands['feather-'. $command];

  drush_dispatch($command, $args);
}

/**
 * Command callback for `drush feather-start`.
 */
function drush_feather_start() {
  $server = feather_get_server();

  $vhosts = $server->get_vhosts();
  if (empty($vhosts)) {
    return drush_set_error('DRUSH_SERVER_NO_VHOSTS', dt('Create at least one virtual host using drush vhost-add before you can start the server.'));
  }

  if ($server->start()) {
    drush_log(dt('Server started with the following virtual hosts:'), 'ok');
    drush_feather_list();
  }
  else {
    return drush_set_error('FEATHER_SERVER_START_ERROR', dt('Unable to start the server.'));
  }
}

/**
 * Command callback for `drush feather-stop`.
 */
function drush_feather_stop() {
  $server = feather_get_server();

  if ($server->stop()) {
    drush_log(dt('Server stopped.'), 'ok');
  }
  else {
    return drush_set_error('FEATHER_SERVER_STOP_ERROR', dt('Unable to stop the server.'));
  }
}

/**
 * Command callback for `drush feather-status`.
 */
function drush_feather_status() {
  $server = feather_get_server();

  $vhosts = $server->get_vhosts();
  if (empty($vhosts)) {
    return drush_set_error('DRUSH_SERVER_NO_VHOSTS', dt('Create at least one virtual host using drush vhost-add before you can start the server.'));
  }

  if ($server->is_running()) {
    drush_log(dt('The server is running, with these virtual hosts:'), 'ok');
    drush_feather_list();
  }
  else {
    drush_log(dt('The server is stopped.'), 'ok');
  }
}

/**
 * Command callback for `drush feather-check-config`.
 */
function drush_feather_check_config() {
  $server = feather_get_server();

  $status = drush_shell_exec($server->apachectl("-t"));
  if (!$status) {
    drush_log(dt('The following errors were found in the server configuration:'), 'error');
    $output = drush_shell_exec_output();
    if (!empty($output)) {
      foreach ($output as $line) {
        drush_print($line);
      }
    }
  }
  else {
    drush_log(dt('No errors found in the server configuration.'), 'ok');
  }

}

/**
 * Command callback for `drush feather-error-log`.
 */
function drush_feather_error_log() {
  $server = feather_get_server();
  $log_file = $server->log_path .'/error_log';
  drush_shell_exec_interactive("tail -f {$log_file}");
}

/**
 * Command callback for `drush feather-list`.
 */
function drush_feather_list() {
  $server = feather_get_server();

  $vhosts = $server->get_vhosts();
  if (empty($vhosts)) {
    drush_log(dt('No virtual host configurations found.'), 'warning');
    return;
  }

  foreach ($vhosts as $name => $vhost) {
    $rows[] = array(
      $vhost->uri,
      '->',
      $vhost->doc_root,
    );
  }

  drush_print_table($rows);
}

/**
 * Command callback for `drush feather-add`.
 */
function drush_feather_add($alias = NULL) {
  $server = feather_get_server();

  if (!empty($alias)) {
    $settings = drush_sitealias_get_record($alias);
  }

  $uri = !empty($settings['uri']) ? $settings['uri'] : drush_get_context('DRUSH_URI', FALSE);
  if (empty($uri)) {
    $host = drush_prompt('Host name', 'localhost');
    $port = drush_prompt('Port', '8080');
    $uri = 'http://'. $host .':'. $port;
  }

  $vhost = $server->vhost_get($uri);
  if ($vhost->exists()) {
    return drush_set_error('DRUSH_TOOLBOX_VHOST_EXISTS', dt('The specified virtual host already exists.'));
  }

  $vhost->doc_root = !empty($settings['root']) ? $settings['root'] : drush_get_context('DRUSH_DRUPAL_ROOT', FALSE);
  if (empty($vhost->doc_root)) {
    $vhost->doc_root = drush_prompt('Document root', getcwd());
  }

  $vhost->save();
}

/**
 * Command callback for `drush feather-remove`.
 */
function drush_feather_remove($alias = NULL) {
  $server = feather_get_server();

  if (!empty($alias)) {
    $settings = drush_sitealias_get_record($alias);
  }

  $uri = !empty($settings['uri']) ? $settings['uri'] : drush_get_context('DRUSH_URI', FALSE);

  // If a uri is passed in, ask for confirmation.
  if (!empty($uri)) {
    $vhost = $server->vhost_get($uri);
    if (!$vhost->exists()) {
      return drush_set_error('DRUSH_TOOLBOX_VHOST_DOES_NOT_EXIST', dt('The specified virtual host configuration does not exist.'));
    }

    if (!drush_confirm(dt('Delete configuration for !uri?', array('!uri' => $vhost->uri)))) {
      // Cancelled.
      return;
    }
  }
  // No uri passed in, prompt the user.
  else {
    $vhosts = $server->get_vhosts();
    if (empty($vhosts)) {
      drush_log(dt('No virtual host configurations found.'), 'warning');
      return;
    }

    $options = array();
    foreach ($vhosts as $name => $vhost) {
      $options[$name] = $vhost->uri;
    }

    $choice = drush_choice($options, 'Choose a virtual host configuration to delete');

    // Cancelled.
    if (empty($choice)) {
      return;
    }

    $vhost = $vhosts[$choice];
  }

  if ($vhost->exists()) {
    $vhost->delete();
    drush_log(dt('!host:!port removed.', array('!host' => $vhost->host, '!port' => $vhost->port)), 'ok');
  }

}

function feather_get_server() {
  static $server;
  if (!isset($server)) {
    feather_include('server.inc');
    $server = new FeatherServer();
  }
  return $server;
}

function feather_get_conf_file() {
  feather_include('conf-file.inc');
  $conf_file = new FeatherHttpdConfFile();
  return $conf_file;
}
