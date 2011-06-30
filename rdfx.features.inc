<?php
/**
 * Implementation of hook_features_export().
 *
 * Defines one or more component types that are available to Features for export
 * and a variety of settings for each type.
 */
function rdf_mappings_features_export($data, &$export, $module_name = '') {
  // Any feature exporting RDF mappings need the rdf and rdfx modules.
  $export['dependencies']['rdf'] = 'rdf';
  $export['dependencies']['rdfx'] = 'rdfx';

  foreach ($data as $name) {
    if ($rdf_mapping = rdf_mapping_load('node', $name)) {
      // Add RDF mapping to available exports.
      $export['features']['rdf_mappings'][$name] = $rdf_mapping;
    }
  }

  return array();
}

/**
 * Implementation of hook_features_export_options().
 *
 * Provides an array of components that can be exported for a given type.
 */
function rdf_mappings_features_export_options() {
  // Only offer node content types for now.
  return node_type_get_names();
}

/**
 * Implementation of hook_features_export_render().
 *
 * Renders a set of components to code as a defaults hook.
 */
function rdf_mappings_features_export_render($module, $data, $export = NULL) {
  $code = array();
  $code[] = '  $rdf_mappings = array();';
  $code[] = '';

  foreach ($data as $k => $name) {
    if (is_array($name)) {
      $name = $k;
    }
    if ($rdf_mapping = rdf_mapping_load('node', $name)) {
      $rdf_mapping_export = features_var_export($rdf_mapping, '  ');
      $rdf_mapping_identifier = features_var_export($name);
      $code[] = "  // Exported RDF mapping: {$name}";
      $code[] = "  \$rdf_mappings['node'][{$rdf_mapping_identifier}] = {$rdf_mapping_export};";
      $code[] = "";
    }
  }

  $code[] = '  return $rdf_mappings;';
  $code = implode("\n", $code);
  return array('rdf_default_mappings' => $code);
}

/**
 * Implementation of hook_features_revert().
 *
 * Reverts components of a feature back to their default state.
 */
function rdf_mappings_features_revert($module) {
  return rdf_mappings_features_rebuild($module);
}

/**
 * Implementation of hook_features_rebuild().
 *
 * Updates faux-exportable components back to their default state.
 */
function rdf_mappings_features_rebuild($module) {
  if ($defaults = features_get_default('rdf_mappings', $module)) {
    foreach ($defaults as $entity_type => $bundles) {
      foreach ($bundles as $bundle => $mapping) {
        rdf_mapping_save(array(
          'type' => 'node',
          'bundle' => $bundle,
          'mapping' => $mapping,
        ));
      }
    }
  }
}
