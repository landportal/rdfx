<?php
// $Id$

//////////////////////////////////////////////////////////////////////////////
// Views API plugins

/**
 * Views style plugin that outputs an RSS 1.0-compatible RDF feed.
 *
 * @see http://web.resource.org/rss/1.0/spec
 * @ingroup views_style_plugins
 */
class views_plugin_style_rdf extends views_plugin_style {
  function attach_to($display_id, $path, $title) {
    $display = $this->view->display[$display_id]->handler;
    $url_options = array();
    if (($input = $this->view->get_exposed_input())) {
      $url_options['query'] = $input;
    }

    $title = !empty($title) ? $title : t('RSS 1.0');
    $url = url($this->view->get_url(NULL, $path), $url_options);
    if ($display->has_path()) {
      if (empty($this->preview)) {
        rdf_add_autodiscovery_link($title, $url, $this->options['format']);
        rdf_add_autodiscovery_link($title, $url, $this->options['format'], array('rel' => 'alternate'));
      }
    }
    else {
      if (empty($this->view->feed_icon)) {
        $this->view->feed_icon = '';
      }
      $this->view->feed_icon .= theme('feed_icon', $url, $title);
      rdf_add_autodiscovery_link($title, $url, $this->options['format']);
      rdf_add_autodiscovery_link($title, $url, $this->options['format'], array('rel' => 'alternate'));
    }
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['description'] = array('default' => '', 'translatable' => TRUE);
    $options['description_from_mission'] = array('default' => '', 'translatable' => TRUE);
    $options['update_period'] = array('default' => '', 'translatable' => TRUE);
    $options['update_frequency'] = array('default' => '', 'translatable' => TRUE);
    $options['update_base'] = array('default' => '', 'translatable' => TRUE);
    $options['format'] = array('default' => RDF_FORMAT, 'translatable' => TRUE);
    return $options;
  }

  function options_form(&$form, &$form_state) {
    module_load_include('inc', 'rdf', 'rdf.admin');
    rdf_admin_feed_form($form, $form_state, $this->options);
  }

  function render() {
    global $base_url;

    if (empty($this->row_plugin)) {
      vpr('views_plugin_style_rdf: Missing row plugin');
      return;
    }

    // Compose the feed title:
    $title = $this->view->display_handler->get_option('sitename_title') ?
      (variable_get('site_name', 'Drupal') . ($slogan = variable_get('site_slogan', '')) ? ' - ' . $slogan : '') :
      $this->view->get_title();

    // Compose the feed description:
    $description = (!empty($this->options['description_from_mission'])) ?
      variable_get('site_mission', '') :
      $this->options['description'];

    // Figure out which display has a path we're using for this feed.
    // If there isn't one, use the global $base_url.
    $link_display_id = $this->view->display_handler->get_link_display(); // FIXME: can return incorrect feed...
    if (!$link_display_id || empty($this->view->display[$link_display_id])) {
      vpr('views_plugin_style_rdf: Missing display ID');
      return;
    }

    // Construct the feed URL:
    $path = $this->view->display[$link_display_id]->handler->get_path();
    $path = $this->view->get_url(NULL, $path);

    // Add any view arguments passed in the URL query string:
    $url_options = array('absolute' => TRUE);
    if (!empty($this->view->exposed_raw_input)) {
      $url_options['query'] = $this->view->exposed_raw_input;
    }

    // Compare the link to the site's default home page; if it is, in fact,
    // the default home page, just use $base_url directly:
    $path = ($path == variable_get('site_frontpage', 'node')) ? '' : $path;
    $link = check_url(url($path, $url_options));

    rdf_register_namespace('rss', 'http://purl.org/rss/1.0/');

    // Construct the channel's metadata:
    $index   = rdf_seq();
    $channel = rdf_denormalize(array(
      $link => array(
        'rdf:type'        => rdf_qname_to_uriref('rss:channel'),
        'rss:title'       => rdf_literal($title, TRUE),
        'rss:link'        => $link, // FIXME: should go to view URL, not feed URL
        'rss:description' => rdf_literal($description, TRUE),
        'rss:items'       => $index->uriref(),
      ),
    ));

    // Support for RSS 1.0 Syndication hints
    // @see http://web.resource.org/rss/1.0/modules/syndication/
    rdf_register_namespace('sy', 'http://purl.org/rss/1.0/modules/syndication/');
    if (!empty($this->options['update_period'])) {
      $channel[] = array($link, 'sy:updatePeriod', check_plain($this->options['update_period']));
    }
    if (!empty($this->options['update_frequency'])) {
      $channel[] = array($link, 'sy:updateFrequency', check_plain($this->options['update_frequency']));
    }
    if (!empty($this->options['update_base'])) {
      $channel[] = array($link, 'sy:updateBase', check_plain($this->options['update_base']));
    }

    // Invoke hook_rdf_views_feed_channel() to allow third-party modules to
    // manipulate the channel metadata:
    foreach (module_implements('rdf_views_feed_channel') as $module) {
      $function = $module . '_rdf_views_feed_channel';
      $function($this, $channel);
    }

    // Compile the channel index and render the items into triples:
    $items = array();
    foreach ($this->view->result as $row) {
      $index[] = url('node/' . $row->nid, array('absolute' => TRUE, 'alias' => TRUE));
      $items = array_merge($items, $this->row_plugin->render($row));
    }

    // Invoke hook_rdf_views_feed() to allow third-party modules to
    // manipulate the full contents of the feed, including the metadata and
    // all constituent feed items. Note that unlike the other two hooks,
    // hook_rdf_views_feed_channel() and hook_rdf_views_feed_item(), the
    // $data argument to this hook is given in normalized form. This makes
    // manipulation easier and may be a good reason to implement this hook
    // instead of the other two which, getting passed the triples in
    // denormalized form, are more suited to easily adding more triples to
    // the channel and the feed items than for manipulating existing data. 
    // Implementers of this hook have final say in the feed output.
    $data = rdf_normalize(array_merge($channel, $index->to_triples(), $items));
    foreach (module_implements('rdf_views_feed') as $module) {
      $function = $module . '_rdf_views_feed';
      $function($this, $data);
    }

    module_load_include('inc', 'rdf', 'rdf.pages');
    rdf_output('feed', $this->options['format'], $data);
    exit;
  }
}
