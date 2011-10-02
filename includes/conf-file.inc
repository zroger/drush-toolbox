<?php

class FeatherConfFile {
  public $filepath;
  public $template_name;

  public $base_path;
  public $conf_path;
  public $log_path;

  public function __construct($filepath, $template) {
    $this->filepath = $filepath;
    $this->template_name = $template;

    $server = feather_get_server();
    $this->base_path = $server->base_path;
    $this->conf_path = $server->conf_path;
    $this->log_path = $server->log_path;

    $this->load();
  }

  public function get_template() {
    // TODO: Allow for passing in a specific template file for overrides.
    $command = drush_get_command();
    $file = $command['path'] .'/templates/'. $this->template_name;
    return file_exists($file) ? $file : FALSE;
  }

  public function load() {
    $this->parse();
  }

  protected function parse() {}

  protected function get_template_variables() {
    $variables = get_object_vars($this);
    return $variables;
  }

  public function save() {
    $variables = $this->get_template_variables();
    $content = $this->render($variables);
    $destination = $this->filepath;

    if (!drush_mkdir(dirname($destination))) {
      return drush_set_error('DRUSH_SERVER_DIR_ERROR', 
        dt('Unable to create directory !dir.', array('dir' => dirname($destination))));
    }
    file_put_contents($destination, $content);
  }

  protected function render($variables = array()) {
    $template = $this->get_template();

    if (empty($template)) {
      return drush_set_error('DRUSH_SERVER_TEMPLATE_NOT_FOUND', 
        dt('Unable to locate template !template.', array('template' => $this->template_name)));
    }

    extract($variables);
    ob_start();
    include $template;
    $content = ob_get_clean();
    return $content;
  }

  public function delete() {
    return unlink($this->filepath);
  }

  public function exists() {
    return file_exists($this->filepath);
  }
}

class FeatherHttpdConfFile extends FeatherConfFile {
  public $host = 'localhost';
  public $ports = array();

  public function __construct() {
    $server = feather_get_server();
    $filepath = $server->conf_path .'/httpd.conf';
    parent::__construct($filepath, 'httpd.tpl.php');
  }

  protected function parse() {
    if (!file_exists($this->filepath)) {
      return;
    }

    $content = file_get_contents($this->filepath);

    // Find the Listen directives for the ports.
    preg_match_all('@^\s*Listen\s+([0-9]+)\s*$@m', $content, $matches);
    if (!empty($matches[1])) {
      $this->ports = $matches[1];
    }

    // ServerName localhost
    preg_match_all('@^\s*ServerName\s+[\'"]?([a-zA-Z0-9.-_]+)[\'"]?\s*$@m', $content, $matches);
    if (!empty($matches[1][0])) {
      $this->host = $matches[1][0];
    }
  }

  protected function get_template_variables() {
    $variables = parent::get_template_variables();

    $server = feather_get_server();
    $vhosts = $server->get_vhosts();
    $ports = array();
    foreach ($vhosts as $key => $vhost) {
      $ports[] = $vhost->port;
    }

    $variables['ports'] = array_unique($ports);
    $variables['vhosts'] = $vhosts;

    return $variables;
  }
}

class FeatherVhostConfFile extends FeatherConfFile {
  public $uri;
  public $host;
  public $port;
  public $doc_root;

  public function __construct($uri) {
    $this->uri = $uri;
    $this->host = parse_url($uri, PHP_URL_HOST);
    $this->port = parse_url($uri, PHP_URL_PORT);
    $this->name = $this->host .':'. $this->port;

    $server = feather_get_server();
    $filepath = "{$server->conf_path}/sites/{$this->name}.conf";

    parent::__construct($filepath, 'vhost.tpl.php');
  }

  protected function parse() {
    if (!file_exists($this->filepath)) {
      return;
    }

    $content = file_get_contents($this->filepath);

    // <VirtualHost *:3000>
    preg_match_all('@^\s*<VirtualHost\s+\*:([0-9]+)\s*$@m', $content, $matches);
    if (!empty($matches[1][0])) {
      $this->port = $matches[1][0];
    }

    // ServerName localhost
    preg_match_all('@^\s*ServerName\s+[\'"]?([a-zA-Z0-9.-_]+)[\'"]?\s*$@m', $content, $matches);
    if (!empty($matches[1][0])) {
      $this->host = $matches[1][0];
    }

    // DocumentRoot "/Users/zroger/treehouse/energy.gov/docroot"
    preg_match_all('@^\s*DocumentRoot\s+[\'"]?([a-zA-Z0-9.\/\-_]+)[\'"]?\s*$@m', $content, $matches);
    if (!empty($matches[1][0])) {
      $this->doc_root = $matches[1][0];
    }
  }

  public function save() {
    parent::save();
    $httpd_conf = feather_get_conf_file();
    $httpd_conf->save();
  }

  public function delete() {
    parent::delete();
    $httpd_conf = feather_get_conf_file();
    $httpd_conf->save();
  }
}
