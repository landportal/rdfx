<?php
// $Id$

//////////////////////////////////////////////////////////////////////////////
// Menu callbacks

function rdf_export_site() {
  $data = rdf_query(url(NULL, array('absolute' => TRUE)));
  rdf_export($data, 'site');
}

function rdf_export_entity($type, $id) {
  $data = rdf_query(url($type .'/'. $id, array('absolute' => TRUE)));
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

  header('Content-Disposition: inline; filename='. $filename);
  header('Content-Type: '. $format->mime_type .'; charset='. $format->encoding);
  header('Content-Length: '. strlen($output));
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
