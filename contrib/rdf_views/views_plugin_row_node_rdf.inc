<?php
// $Id$

//////////////////////////////////////////////////////////////////////////////
// Views API plugins

/**
 * Views row plugin that invokes node_view() on each node and prepares its
 * body for inclusion as an RSS 1.0 item description in the feed.
 *
 * @see http://web.resource.org/rss/1.0/spec
 * @ingroup views_row_plugins
 */
class views_plugin_row_node_rdf extends views_plugin_row {
  function option_definition() {
    $options = parent::option_definition();
    $options['item_length'] = array('default' => 'default');
    return $options;
  }

  function options_form(&$form, &$form_state) {
    $form['item_length'] = array(
      '#type' => 'select',
      '#title' => t('Content display type'),
      '#options' => array(
        'fulltext' => t('Full text'),
        'teaser'   => t('Title plus teaser'),
        'title'    => t('Title only'),
        'default'  => t('Use default RSS settings'),
      ),
      '#default_value' => $this->options['item_length'],
    );
  }

  function render($row) {
    // Load the specified node:
    $item = node_load($row->nid);
    $item->build_mode = NODE_BUILD_RSS;
    $item->url  = url('node/'. $row->nid, array('absolute' => TRUE, 'alias' => TRUE));
    $item->link = url('node/'. $row->nid, array('absolute' => TRUE));

    // Apply input filters and prepare the node teaser, also allowing other
    // $modules to change node->teaser before viewing:
    if (($item_length = $this->options['item_length']) == 'default') {
      $item_length = variable_get('feed_item_length', 'teaser');
    }

    $teaser = ($item_length == 'teaser');
    $item = node_hook($item, 'view') ? node_invoke($item, 'view', $teaser, FALSE) : node_prepare($item, $teaser);
    node_invoke_nodeapi($item, 'view', $teaser, FALSE);

    // Make sure the item has all the basic mandatory RSS 1.0 properties:
    // @see http://web.resource.org/rss/1.0/spec#s5.5
    $data = array(
      array($item->url, 'rdf:type',  rdf_qname_to_uriref('rss:item')),
      array($item->url, 'rss:title', $item->title),
      array($item->url, 'rss:link',  $item->link), // NOTE: it actually *is* defined as an xsd:string property, not a URI.
    );

    // Include the node body using a rss:description property unless "Title
    // only" was selected in the row style configuration. This property is
    // optional in the spec so leaving it out is fine.
    if ($item_length != 'title') {
      // Prepare the item description:
      switch ($item_length) {
        case 'fulltext':
          $item_text = trim($item->body);
          break;
        case 'teaser':
          $item_text = trim($item->teaser);
          if (!empty($item->readmore)) {
            $item_text .= '<p>'. l(t('read more'), 'node/'. $item->nid, array('absolute' => TRUE, 'attributes' => array('target' => '_blank'))) .'</p>';
          }
          break;
      }
      if ($item_text != '') {
        $data[] = array($item->url, 'rss:description', $item_text);
      }
    }

    // Add dc:date for the created date:
    // @see http://web.resource.org/rss/1.0/modules/dc/
    $data[] = array($item->url, 'dc:date', rdf_datetime($item->created));

    // Add dc:subject properties for the node's taxonomy terms.
    // @see http://web.resource.org/rss/1.0/modules/dc/
    if (!empty($item->taxonomy)) {
      foreach ($item->taxonomy as $tid => $term) {
        $data[] = array($item->url, 'dc:subject', $term->name);
      }
    }

    // Query the RDF API for all available information on this node. The RDF
    // Schema module takes care of mapping core fields and CCK fields to RDF
    // properties for us to use. Note that we remove any returned rdf:type
    // properties as we want these nodes to be explicitly typed to rss:item.
    $skip_predicates = array_map('rdf_qname_to_uri', array('rdf:type'));
    foreach (rdf_query($item->url) as $statement) {
      list($s, $p, $o) = $statement;
      if (!in_array((string)$p, $skip_predicates)) { // skip rdf:type
        $data[] = array($s, $p, $o);
      }
    }

    // HACK: this snippet provides direct support for RDF-mapped CCK fields. 
    // This should eventually be properly handled by the RDF Schema module.
    if (isset($item->content) && is_array($item->content)) {
      foreach ($item->content as $field_name => $field_info) {
        if (($field_uri = variable_get('rdf_schema_property_content_'. $field_name, '')) != '' && rdf_is_valid_uri($field_uri)) {
          if (isset($item->$field_name)) {
            $field_values = is_array($item->$field_name) ? $item->$field_name : array($item->$field_name);
            foreach ($field_values as $field_value) {
              if (($value = $field_value['value']) != '') { // skip empty values

                // Cast CCK date field values into the correct RDF datatype:
                if (isset($field_value['date_type'])) {
                  $value = rdf_datetime($value); // FIXME: offset by $field_value['timezone'] ?
                }
                $data[] = array($item->url, $field_uri, $value);
              }
            }
          }
        }
      }
    }

    // HACK: this snippet provides direct support for the Location module.
    if (isset($item->location)) {
      rdf_register_namespace('geo', 'http://www.w3.org/2003/01/geo/wgs84_pos#');
      if (isset($item->location['latitude'])) {
        $data[] = array($item->url, 'geo:lat', $item->location['latitude']);
      }
      if (isset($item->location['longitude'])) {
        $data[] = array($item->url, 'geo:long', $item->location['longitude']);
      }
    }

    // Invoke hook_rdf_views_feed_item() to allow third-party modules to
    // manipulate the already collected triples as well as add any further
    // triples to the feed item. They can also do this by implementing the
    // RDF Schema hooks, of course, but this hook provides the ultimate
    // flexibility in deciding the composition of the RDF feeds' items.
    foreach (module_implements('rdf_views_feed_item') as $module) {
      $function = $module .'_rdf_views_feed_item';
      $function($this, $item, $data);
    }

    return $data;
  }
}
