<?php
// $Id$

//////////////////////////////////////////////////////////////////////////////
// Views API hooks

/**
 * Implementation of hook_views_plugins().
 */
function rdf_views_plugins() {
  return array(
    // Style plugins
    'style' => array(
      'rdf' => array(
        'title' => t('RDF Feed'),
        'help' => t('Generates an RSS 1.0-compatible RDF feed from a view.'),
        'handler' => 'views_plugin_style_rdf',
        'path' => drupal_get_path('module', 'rdf') .'/contrib/rdf_views',
        'uses row plugin' => TRUE,
        'uses options' => TRUE,
        'type' => 'feed',
        'help topic' => 'style-rss',
      ),
    ),

    // Row plugins
    'row' => array(
      'node_rdf' => array(
        'title' => t('Node (RDF)'),
        'help' => t('Outputs the node as an RDF 1.0-compatible RDF resource.'),
        'handler' => 'views_plugin_row_node_rdf',
        'path' => drupal_get_path('module', 'rdf') .'/contrib/rdf_views',
        'base' => array('node'), // only works with 'node' as base.
        'uses options' => TRUE,
        'type' => 'feed',
        'help topic' => 'style-node-rss',
      ),
    ),
  );
}
