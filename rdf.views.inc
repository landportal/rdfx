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
        'path' => drupal_get_path('module', 'rdf') . '/contrib/rdf_views',
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
        'path' => drupal_get_path('module', 'rdf') . '/contrib/rdf_views',
        'base' => array('node'), // only works with 'node' as base.
        'uses options' => TRUE,
        'type' => 'feed',
        'help topic' => 'style-node-rss',
      ),
    ),
  );
}

/**
 * Implementation of hook_views_data()
 */
function rdf_views_data() {
  $data = array(
    'rdf_resources' => array(
      // Table info.
      'table' => array(
        'group' => t('RDF'),
        'base' => array(
          'field' => 'rid',
          'title' => t('RDF'),
          'help' => t('RDF meta-data.'),
          'weight' => 0,
        ),
      ),
      // Fields.
      'uri' => array(
        'title' => t('URI'),
        'help' => t('The URI of the RDF resource'),
        'field' => array('handler' => 'views_handler_field'),
        'filter' => array('handler' => 'views_handler_filter_boolean_operator'), 
      ),
    ),
  );
  
  foreach (rdf_db_get_repository_names() as $name => $table) {
    $info = rdf_db_load_repository($name);
    $data[$table] = array(
      // Table info.
      'table' => array(
        'group' => t('@table', array('@table' => $info->title)),
        'join' => array(
          'rdf_resources' => array(
            'left_field' => 'rid',
            'field' => 'sid',
          ),
        ),
      ),
      // Fields.
      /* TODO: 'oid', 'tid', 'lang' */
      'sid' => array(
        'title' => t('Subject'),
        'help' => t("Subject of the RDF statement."),
        'field' => array('handler' => 'views_handler_field'),
      ),
      'uid' => array(
        'title' => t('User'),
        'help' => t("User who created statement."),
        'field' => array('handler' => 'views_handler_field'),
        'relationship' => array(
          'handler' => 'views_handler_relationship',
          'base' => 'users',
          'field' => 'uid',
          'label' => t('user'),
        ),
      ),
      'created' => array(
        'title' => t('Created'),
        'help' => t("Timestamp of the statement."),
        'field' => array('handler' => 'views_handler_field_date'),
        'filter' => array('handler' => 'views_handler_filter_date'),
      ),
      'data' => array(
        'title' => t('Object'),
        'help' => t('RDF Object literal'),
        'field' => array('handler' => 'views_handler_field_rdf_literal'),
        'relationship' => array(
          'handler' => 'views_handler_relationship',
          'base' => 'rdf_resources',
          'base field' => 'uri',
          'field' => 'data',
          'label' => t('@table Resources', array('@table' => $info->title)),
        ),
      ),
    );
    // Add predicate and named graph fields differently as we need to refer back
    // to rdf_resources to pull usable values.
    $data[$table .'_predicate']['table'] = array(
      'group' => t('@table', array('@table' => $info->title)),
      'join' => array(
        'rdf_resources' => array(
          'left_table' => $table,
          'left_field' => 'pid',
          'table' => 'rdf_resources',
          'field' => 'rid',
        ),
      ),
    );
     $data[$table .'_predicate']['uri'] = array(
        'title' => t('Predicate'),
        'help' => t("The RDF statement's predicate."),
        'field' => array('handler' => 'views_handler_field'),
        'filter' => array('handler' => 'views_handler_filter_rdf_predicate'),
    );
    $data[$table .'_context']['table'] = array(
      'group' => t('@table', array('@table' => $info->title)),
      'join' => array(
        'rdf_resources' => array(
          'left_table' => $table,
          'left_field' => 'gid',
          'table' => 'rdf_resources',
          'field' => 'rid',
        ),
      ),
    );
     $data[$table .'_context']['uri'] = array(
        'title' => t('Named graph'),
        'help' => t("The RDF statement's named graph."),
        'field' => array('handler' => 'views_handler_field'),
        'filter' => array('handler' => 'views_handler_filter_rdf_context'),
    );
  }
  return $data;
}

function rdf_views_handlers() {
  return array(
    'info' => array('path' => drupal_get_path('module', 'rdf') . '/contrib/rdf_views'),
    'handlers' => array(
      'views_handler_filter_rdf_predicate' => array(
        'parent' => 'views_handler_filter_in_operator',
      ),
      'views_handler_filter_rdf_context' => array(
        'parent' => 'views_handler_filter_in_operator',
      ),
      'views_handler_field_rdf_literal' => array(
        'parent' => 'views_handler_field',
      ),
    ),
  );
}
