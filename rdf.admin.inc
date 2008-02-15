<?php
// $Id$
/**
 * rdf.admin.inc - RDF API admin page callbacks.
 *
 * @author Arto Bendiken <http://bendiken.net/>
 * @copyright Copyright (c) 2007-2008 Arto Bendiken. All rights reserved.
 * @license GPL <http://creativecommons.org/licenses/GPL/2.0/>
 * @package rdf.module
 */

//////////////////////////////////////////////////////////////////////////////
// RDF settings form

function rdf_admin_settings() {
  $form = array();

  $formats = array();
  foreach (rdf_get_formats('names') as $key => $value) {
    $formats[$key] = '';
  }
  $form['formats'] = array('#type' => 'fieldset', '#title' => t('Formats'), '#collapsible' => TRUE, '#collapsed' => FALSE, '#description' => rdf_help('admin/settings/rdf#formats'));
  $form['formats']['rdf_format'] = array('#type' => 'radios', '#options' => $formats, '#default_value' => RDF_FORMAT);

  return array_merge(system_settings_form($form), array('#theme' => 'rdf_admin_settings'));
}

function theme_rdf_admin_settings($form) {
  $head = array(t('Default'), t('Name'), t('MIME type'), t('Readable'), t('Writable'));
  $rows = array();
  foreach (rdf_get_formats() as $format) {
    $rows[] = array(
      drupal_render($form['formats']['rdf_format'][$format->name]),
      !empty($format->link) ? l($format->title, $format->link) : $format->title,
      $format->mime_type,
      !empty($format->unserialize) ? t('Yes') : t('No'),
      !empty($format->serialize) ? t('Yes') : t('No')
    );
  }

  $form['formats']['#value'] = theme('table', $head, $rows, array('class' => 'formats'));
  unset($form['formats']['rdf_format']);

  return drupal_render($form);
}

//////////////////////////////////////////////////////////////////////////////
// RDF data management

function rdf_admin_data() {
  $subject   = isset($_GET['s']) ? urldecode($_GET['s']) : url(NULL, array('absolute' => TRUE));
  $predicate = isset($_GET['p']) ? urldecode($_GET['p']) : '';
  $object    = isset($_GET['o']) ? urldecode($_GET['o']) : '';

  $form['query'] = array('#type' => 'fieldset', '#title' => t('Query'), '#prefix' => '<div class="container-inline">', '#suffix' => '</div>');
  $form['query']['subject'] = array('#type' => 'textfield', '#title' => '', '#default_value' => $subject, '#maxlength' => 255, '#size' => 20);
  $form['query']['predicate'] = array('#type' => 'textfield', '#title' => '', '#default_value' => $predicate, '#maxlength' => 255, '#size' => 20);
  $form['query']['object'] = array('#type' => 'textfield', '#title' => '', '#default_value' => $object, '#maxlength' => 255, '#size' => 20);
  $form['query']['submit'] = array('#type' => 'submit', '#value' => t('Find'), '#submit' => array('rdf_admin_data_submit'));

  return $form;
}

function rdf_admin_data_submit($form, &$form_state) {
  extract($form_state['values'], EXTR_SKIP | EXTR_REFS);

  $form_state['redirect'] = array('admin/content/rdf', drupal_query_string_encode(array('s' => $subject, 'p' => $predicate, 'o' => $object)));
}

function theme_rdf_admin_data($form) {
  $subject   = $form['query']['subject']['#value'];
  $predicate = $form['query']['predicate']['#value'];
  $object    = $form['query']['object']['#value'];
  $data      = rdf_normalize(rdf_query($subject, $predicate, $object));

  $output = drupal_render($form['query']);
  $output .= theme('rdf_triple_table', $data, array('link' => 'theme_rdf_admin_link'));
  $output .= drupal_render($form);
  return $output;
}

function theme_rdf_admin_link($title, $uri, array $attributes = array()) {
  return l($title, 'admin/content/rdf', array('query' => drupal_query_string_encode(array('s' => $uri))));
}

//////////////////////////////////////////////////////////////////////////////
// RDF namespace management

function rdf_admin_namespaces() {
  $header = array(t('Prefix'), t('Base URI'), array('data' => t('Operations'), 'colspan' => '2'));

  // FIXME: need something more loosely-coupled and robust for this:
  $mutables = module_exists('rdf_db') ? rdf_db_rdf_namespaces() : array();

  $rows = array();
  foreach (rdf_get_namespaces() as $prefix => $uri) {
    $mutable = isset($mutables[$prefix]);
    $rows[] = array(
      $prefix, //l($prefix, 'admin/settings/rdf/predicates/' . $prefix),
      check_plain($uri),
      !$mutable ? '' : l(t('edit'), 'admin/settings/rdf/namespaces/edit/' . $prefix),
      !$mutable ? '' : l(t('delete'), 'admin/settings/rdf/namespaces/delete/' . $prefix),
    );
  }

  return theme('table', $header, $rows);
}

//////////////////////////////////////////////////////////////////////////////
// RDF repository management

function rdf_admin_repositories() {
  $header = array(t('Name'), t('Statements'), t('Persistent'), t('Mutable'), array('data' => t('Operations'), 'colspan' => '2'));

  $rows = array();
  foreach (rdf_get_repositories() as $name => $info) {
    $mutable = !empty($info['mutable']) && $name != 'default'; // FIXME
    $rows[] = array(
      l($info['title'], 'admin/settings/rdf/repositories/edit/' . $name,
        array('attributes' => array('title' => @$info['dc:description']))),
      $name == 'system' ? t('n/a') : (empty($info['statements']) ? '-' : number_format($info['statements'])),
      !empty($info['persistent']) ? t('Yes') : t('No'),
      !empty($info['mutable']) ? t('Yes') : t('No'),
      !$mutable ? '' : l(t('edit'), 'admin/settings/rdf/repositories/edit/' . $name),
      !$mutable ? '' : l(t('delete'), 'admin/settings/rdf/repositories/delete/' . $name),
    );
  }

  return theme('table', $header, $rows);
}
