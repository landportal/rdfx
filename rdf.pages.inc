<?php
// $Id$

//////////////////////////////////////////////////////////////////////////////
// Menu callbacks

function rdf_export_site() {
  $data = rdf_query(url(NULL, array('absolute' => TRUE)));
  rdf_export($data, 'site');
}

function rdf_export_entity($type, $id) {
  $data = rdf_query(url($type . '/' . $id, array('absolute' => TRUE)));
  rdf_export($data, implode('-', array($type, $id)));
}

/**
 * @deprecated replace with rdf_output().
 */
function rdf_export($data, $filename = 'export', $format = RDF_FORMAT, $options = array()) {
  //module_load_include('inc', 'rdf_export', 'rdf_export');

  $formats = rdf_get_formats();
  $format = isset($formats[@$_GET['format']]) ? $_GET['format'] : $format;
  $format = $formats[$format];
  $filename = implode('.', array($filename, $format->file_ext));

  if (!empty($options['log'])) {
    watchdog('rdf', 'Exported RDF data file: %filename.', array('%filename' => $filename));
  }

  $output = rdf_serialize(is_string($data) ? $data() : $data, array('format' => $format->name));

  header('Content-Disposition: inline; filename=' . $filename);
  header('Content-Type: ' . $format->mime_type . '; charset=' . $format->encoding);
  header('Content-Length: ' . strlen($output));
  die($output);
}

function rdf_output($filename, $format, $data, array $options = array()) {
  global $base_url, $language;

  // Figure out the serialization format and compose the filename
  $formats  = rdf_get_formats();
  $format   = !empty($format) ? $format : RDF_FORMAT;
  $format   = isset($formats[$format]) ? $formats[$format] : reset($formats); // TODO: auto-negotiation    
  $filename = !empty($filename) ? $filename : 'drupal';
  $filename = implode('.', array($filename, $format->file_ext));

  // Merge in the default options 
  $defaults = array(
    'content_disposition'  => 'inline',
    'content_type'         => defined('TRACE_TEXT_OUTPUT') ? 'text/plain' : $format->mime_type,
    'content_type_charset' => $format->encoding,
  );
  $options = array_merge($defaults, $options);

  // Serialize the RDF data                              
  $content = rdf_serialize($data, array(
    'format'   => $format->name,
    'language' => isset($options['language']) ? $options['language'] : $language->language,
  ));

  // Compose HTTP response headers
  $options['http_headers'] = !empty($options['http_headers']) ? $options['http_headers'] : array();
  $options['http_headers'] = array_merge($options['http_headers'], array(
    'Content-Disposition' => $options['content_disposition'] . '; filename=' . $filename,
    'Content-Type'        => $options['content_type'] . '; charset=' . $options['content_type_charset'],
    'Content-Length'      => strlen($content),
  ));

  // Output HTTP response headers and content 
  if (!headers_sent()) {
    if (isset($options['http_status'])) {
      drupal_set_header('HTTP/1.1 ' . $options['http_status']);
    }
    foreach ($options['http_headers'] as $k => $vs) {
      foreach (is_array($vs) ? $vs : array($vs) as $v) {
        drupal_set_header($k . ': ' . $v);
      }
    }
  }
  print $content;
}

//////////////////////////////////////////////////////////////////////////////
// Menu callbacks for RDF feeds

function rdf_feed_callback($feed_id) {
  $feed = rdf_get_feed_info($feed_id);
  $feed->arguments = array_slice(func_get_args(), 1);

  // TODO
  return call_user_func_array($feed->callback, $feed->arguments);
}

/**
 * Overrides node.module's RSS output.
 */
function rdf_feed_node_frontpage() {
  // TODO
  return node_feed();
}

/**
 * Overrides taxonomy.module's RSS output.
 */
function rdf_feed_taxonomy_term($str_tids = '', $depth = 0, $op = 'page') {
  // TODO
  if ($op != 'feed' || TRUE) {
    module_load_include('inc', 'taxonomy', 'taxonomy.pages'); // taxonomy.pages.inc
    return taxonomy_term_page($str_tids, $depth, $op);
  }
}

/**
 * Overrides blog.module's RSS output.
 */
function rdf_feed_blog_user($account) {
  // TODO
  module_load_include('inc', 'blog', 'blog.pages'); // blog.pages.inc
  return blog_feed_user($account);
}

/**
 * Overrides blog.module's RSS output.
 */
function rdf_feed_blog_last() {
  // TODO
  module_load_include('inc', 'blog', 'blog.pages'); // blog.pages.inc
  return blog_feed_last();
}

/**
 * Overrides aggregator.module's RSS output.
 */
function rdf_feed_aggregator_rss() {
  // TODO
  module_load_include('inc', 'aggregator', 'aggregator.pages'); // aggregator.pages.inc
  return aggregator_page_rss();
}

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
