<?php
// $Id$

class views_handler_filter_rdf_context extends views_handler_filter_in_operator {
  function get_value_options() {
  	
  	// rdf_db_rdf_contexts() won't work because we need the internal gids, this
  	// then is a slightly modified recreation.
  	$contexts = array();
   	foreach (rdf_db_get_repository_names() as $name => $table) {
   	  $result = db_query('SELECT DISTINCT g.uri, g.rid gid FROM {'. db_escape_table($table) .'} d INNER JOIN {rdf_resources} g ON d.gid = g.rid');
   	  while ($row = db_fetch_object($result)) {
   	    $contexts[$row->gid] = $row->uri;
   	  }
   	}

    $options = array();
    foreach ($contexts as $k => $v) {
      if ($label = rdf_query($v, 'rdfs:label', NULL)->to_array()) {
        $label = array_pop($label);
        $options[$k] = $label[2];
      }
      else {
        $options[$k] = $v;
      }
    }
    $this->value_options = $options;
  }
}
