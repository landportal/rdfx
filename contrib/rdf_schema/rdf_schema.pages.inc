<?php
// $Id$

//////////////////////////////////////////////////////////////////////////////
// Menu callbacks

function rdf_schema_export_settings($prefix = NULL) {
  if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    return rdf_schema_import_settings();
  }

  $data = array();
  $result = $prefix ?
    db_query('SELECT name, value FROM {variable} WHERE name LIKE \'%s\' ORDER BY name, value', str_replace('*', '%', $prefix) .'%') :
    db_query('SELECT name, value FROM {variable} ORDER BY name, value');
  while ($variable = db_fetch_object($result)) {
    $subject = url('rdf/variable/'. $variable->name, array('absolute' => TRUE));

    if (strlen($variable->value) > 3000) {
      continue; // HACK: the ARC Turtle parser currently chokes on literals longer than ~3000 characters
    }

    $data[$subject] = array(
      rdf::type   => array(rdf_uri('http://drupal.org/rdf/variable')),
      rdfs::label => array($variable->name),
      rdf::value  => array(rdf_literal($variable->value, NULL, 'http://purl.org/php/serialized')), // FIXME
    );
  }
  rdf_schema_export($data, 'settings', 'ntriples');
}

function rdf_schema_export_modules($prefix = NULL) {
  if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    return rdf_schema_import_modules();
  }

  $data = array();
  $result = $prefix ?
    db_query('SELECT name, status FROM {system} WHERE type = \'module\' AND name LIKE \'%s\' ORDER BY name, status', str_replace('*', '%', $prefix) .'%') :
    db_query('SELECT name, status FROM {system} WHERE type = \'module\' ORDER BY name, status');
  while ($module = db_fetch_object($result)) {
    $subject = url('rdf/module/'. $module->name, array('absolute' => TRUE));
    $data[$subject] = array(
      rdf::type   => array(rdf_uri('http://drupal.org/rdf/module')),
      rdfs::label => array($module->name),
      'http://drupal.org/rdf/terms#status' => array(rdf_literal($module->status, NULL, 'xsd:boolean')),
    );
  }
  rdf_schema_export($data, 'modules', 'ntriples');
}

function rdf_schema_export($data, $filename = 'export', $format = RDF_FORMAT, $options = array()) {
  $formats = rdf_get_formats();
  $format = isset($formats[@$_GET['format']]) ? $_GET['format'] : $format;
  $format = $formats[$format];
  $filename = implode('.', array($filename, $format->file_ext));

  $output = rdf_serialize(is_string($data) ? $data() : $data, array('format' => $format->name));

  $format->mime_type = 'text/plain'; // FIXME
  header('Content-Disposition: inline; filename='. $filename);
  header('Content-Type: '. $format->mime_type .'; charset='. $format->encoding);
  header('Content-Length: '. strlen($output));
  die($output);
}

function rdf_schema_import_settings() {
  $input = file_get_contents('php://input');
  $input = rdf_normalize(rdf_unserialize($input, array('format' => 'ntriples')));

  $config = array();
  foreach ($input as $data) {
    if ((string)$data[rdf::type][0] == 'http://drupal.org/rdf/variable') {
      if (($name = $data[rdfs::label][0]) && isset($data[rdf::value])) {
        $value = unserialize($data[rdf::value][0]->value);
        $config[$name] = $value;
      }
    }
  }

  foreach ($config as $name => $value) {
    variable_set($name, $value);
  }

  printf("OK (imported %d variables)\n", count($config));
}

function rdf_schema_import_modules() {
  include_once './includes/install.inc';

  $input = file_get_contents('php://input');
  $input = rdf_normalize(rdf_unserialize($input, array('format' => 'ntriples')));

  $modules = array();
  foreach ($input as $data) {
    if ((string)$data[rdf::type][0] == 'http://drupal.org/rdf/module') {
      if (($name = $data[rdfs::label][0]) && isset($data['http://drupal.org/rdf/terms#status'])) {
        $enabled = $data['http://drupal.org/rdf/terms#status'][0];
        $enabled = !empty($enabled) && (bool)$enabled->value;
        if ($enabled && drupal_get_installed_schema_version($name) == SCHEMA_UNINSTALLED) {
          $modules['install'][] = $name;
        }
        else {
          $modules[$enabled ? 'enable' : 'disable'][] = $name;
        }
      }
    }
  }

  if (!empty($modules['enable'])) {
    module_enable($modules['enable']);
  }

  if (!empty($modules['disable'])) {
    module_disable($modules['disable']);
  }

  if (!empty($modules['install'])) {
    foreach ($modules['install'] as $key => $module) {
      if (!drupal_check_module($module)) {
        unset($modules['install'][$key]);
      }
    }
    drupal_install_modules($modules['install']);
  }

  drupal_clear_css_cache();
  drupal_clear_js_cache();

  // Notify locale module about module changes, so translations can be
  // imported. This might start a batch, and only return to the redirect
  // path after that.
  module_invoke('locale', 'system_update', $modules['install']);

  // Synchronize to catch any actions that were added or removed.
  actions_synchronize();

  printf("OK\n");
}
