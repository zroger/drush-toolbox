<?php

class FeatherServer {
  protected $attributes = array(
    'base_path' => NULL,
    'conf_path' => NULL,
    'log_path' => NULL,
    'conf_file' => NULL,
  );

  public function __construct() {
  }

  /**
   * Attribute getter.  Allows for a get_ATTRIBUTE() method, otherwise falls
   * back to returning the value of the key in the attributes array.
   */
  public function __get($key) {
    $getter = 'get_'. $key;
    if (is_callable(array($this, $getter))) {
      return $this->$getter();
    }

    if (array_key_exists($key, $this->attributes)) {
      return $this->attributes[$key];
    }
  }

  /**
   * Attribute setter.  Only attributes with a coorresponding set_ATTRIBUTE
   * method are settable.
   */
  public function __set($key, $value) {
    $setter = 'set_'. $key;
    if (is_callable(array($this, $setter))) {
      $this->$setter($value);
    }
  }

  /**
   * Handle isset() or empty() calls on attributes.
   */
  public function __isset($key) {
    return isset($this->attributes[$key]);
  }

  /**
   * Handle unset() calls on attributes.
   */
  public function __unset($key) {
    unset($this->attributes[$key]);
  }

  /**
   * Getter for $conf_file.
   */
  public function get_conf_file() {
    return feather_get_conf_file();
  }

  /**
   * Getter for $base_path.
   */
  public function get_base_path() {
    $base_path = drush_server_home() .'/.drush-server';
    return $this->check_dir($base_path);
  }

  /**
   * Getter for $conf_path.
   */
  public function get_conf_path() {
    return $this->check_dir($this->base_path .'/conf');
  }

  /**
   * Getter for $log_path.
   */
  public function get_log_path() {
    return $this->check_dir($this->base_path .'/log');
  }

  protected function check_dir($path) {
    if (!drush_mkdir($path)) {
      return drush_set_error('DRUSH_SERVER_DIR_ERROR', dt('Unable to create directory !dir.', array('dir' => $path)));
    }
    return $path;
  }

  public function is_running() {
    $vhosts = $this->get_vhosts();
    if (empty($vhosts)) {
      return FALSE;
    }

    foreach ($vhosts as $vhost) {
      $url = "http://{$vhost->host}:{$vhost->port}/server-status?auto";
      $running = drush_shell_exec("curl -s %s", $url);
      if ($running) {
        return TRUE;
      }
    }
    return FALSE;
  }

  public function get_vhosts() {
    $sites_path = $this->conf_path .'/sites';
    $files = drush_scan_directory($sites_path, '/\:[0-9]+\.conf/');

    $vhosts = array();
    foreach ($files as $file) {
      $uri = 'http://'. $file->name;
      $vhosts[$file->name] = $this->vhost_get($uri);
    }
    return $vhosts;
  }

  public function start() {
    $command = $this->apachectl('-k start');

    $status = drush_shell_exec($command);

    $output = drush_shell_exec_output();
    if (!empty($output)) {
      foreach ($output as $line) {
        // TODO: parse the output from apachectl to get better messages.
        drush_log($line, 'warning');
      }
    }

    return $status;
  }

  public function stop() {
    $command = $this->apachectl('-k stop');

    $status = drush_shell_exec($command);

    $output = drush_shell_exec_output();
    if (!empty($output)) {
      foreach ($output as $line) {
        // TODO: parse the output from apachectl to get better messages.
        drush_log($line, 'warning');
      }
    }

    return $status;
  }

  public function apachectl() {
    $args = func_get_args();
    $command = "apachectl -f \"{$this->conf_file->filepath}\" " . join(' ', $args);
    return $command;
  }

  function vhost_get($uri) {
    feather_include('conf-file.inc');
    $vhost = new FeatherVhostConfFile($uri);
    return $vhost;
  }
}
