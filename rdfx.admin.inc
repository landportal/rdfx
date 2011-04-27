<?php

/**
 * Callback function for viewing all bundles' RDF mappings.
 */
function rdfx_mapping_overview() {
  return '';
}

/**
 * Menu callback for viewing all declared namespaces and their prefixes.
 */
function rdfx_admin_namespaces() {
  $output = '';

  // Builds a table of existing namespaces known to the system.
  $table_namespaces = array();
  $table_namespaces['header'] = array('Prefix', 'Namespace');
  foreach (rdf_get_namespaces() as $prefix => $namespace) {
    $table_namespaces['rows'][] = array($prefix, $namespace);
  }
  $output .= theme('table', $table_namespaces);

  // Form to add namespaces.
  $form = drupal_get_form('rdfx_admin_namespaces_form');
  $output .= drupal_render($form);

  return $output;
}

function rdfx_admin_namespaces_form($form, &$form_state) {
  $form['prefix'] = array(
    '#type' => 'textfield',
    '#title' => t('Prefix'),
    '#required' => TRUE,
    '#description' => t('Choose a prefix for this namespace, e.g. dc, foaf, sioc. This prefix will be used as an abbreviation for the namespace URI.'),
  );
  $form['ns_uri'] = array(
    '#type' => 'textfield',
    '#title' => t('Namespace URI'),
    '#required' => TRUE,
    '#default_value' => isset($form_state['values']['ns_uri']) ? $form_state['values']['ns_uri'] : NULL,
    '#description' => t("Enter the URI of the namespace. Make sure it ends with either / or #."),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}

function rdfx_admin_namespaces_form_validate($form, &$form_state) {
  // Loads the XML Namespace regular expression patterns.
  module_load_include('inc', 'rdfx');

  $prefix = $form_state['values']['prefix'];
  $ns_uri = $form_state['values']['ns_uri'];

  // Ensures that the namespace is a valid URI.
  if (!valid_url($ns_uri, $absolute = TRUE)) {
    form_set_error('ns_uri', t('The namespace URI must be a valid URI.'));
  }
  // Ensures the namespace URI ends in either / or #.
  if (!preg_match('/(\/|\#)$/', $ns_uri)) {
    form_set_error('ns_uri', t('The namespace URI must end in either a / or a #.'));
  }
  // Ensures the prefix is well formed according to the specification.
  if (!preg_match('/^' . PREFIX .'$/', $prefix)) {
    form_set_error('prefix', t('The prefix must follow the !link.', array('!link' => '<a href="http://www.w3.org/TR/xml-names11/#NT-NCName">XML Namespace Specification</a>')));
  }
}

function rdfx_admin_namespaces_form_submit($form, &$form_state) {
  $prefix = $form_state['values']['prefix'];
  $ns_uri = $form_state['values']['ns_uri'];
  // Prepares a fake empty vocabulary for _rdfx_save_vocabulary() to save the
  // namespace and prefix.
  // @todo use API when http://drupal.org/node/1117646 is fixed.
  $vocabulary = array(
    'title' => array(),
    'description' => array(),
    'namespaces' => array(),
  );
  _rdfx_save_vocabulary($ns_uri, $prefix, $vocabulary);
  drupal_set_message(t('The namespace @namespace has been saved with the prefix @prefix.', array('@namespace' => $ns_uri, '@prefix' => $prefix)));
}
