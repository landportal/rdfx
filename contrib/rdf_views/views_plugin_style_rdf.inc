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
    $options['format'] = array('default' => RDF_FORMAT, 'translatable' => TRUE);
    return $options;
  }

  function options_form(&$form, &$form_state) {
    $form['description_from_mission'] = array(
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['description_from_mission']),
      '#title' => t('Use the site mission as the channel description'),
    );
    $form['description'] = array(
      '#type' => 'textfield',
      '#title' => t('RSS 1.0 channel description'),
      '#default_value' => $this->options['description'],
      '#description' => t('This will appear as the channel description in the RDF feed.'),
      '#process' => array('views_process_dependency'),
      '#dependency' => array('edit-style-options-override' => array(FALSE)),
    );
    $form['format'] = array(
      '#type' => 'select',
      '#title' => t('Default format'),
      '#default_value' => $this->options['format'],
      '#options' => rdf_get_formats('names'),
      '#description' => t('This determines the default RDF serialization format to output. The client can override this by passing an <code>Accept</code> header specifying another supported format.'),
    );
  }

  function render() {
    global $base_url, $language;

    if (empty($this->row_plugin)) {
      vpr('views_plugin_style_rdf: Missing row plugin');
      return;
    }

    $formats  = rdf_get_formats();
    $format   = $formats[$this->options['format']];

    // Compose the feed title:
    $title = $this->view->display_handler->get_option('sitename_title') ?
      (variable_get('site_name', 'Drupal') . ($slogan = variable_get('site_slogan', '')) ? ' - '. $slogan : '') :
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

    // TODO: rdf_register_namespace('rss', 'http://purl.org/rss/1.0/');

    // Construct the channel's metadata:
    $index   = rdf_seq();
    $channel = rdf_denormalize(array(
      $link => array(
        'rdf:type'        => rdf_qname_to_uriref('rss:channel'),
        'rss:title'       => $title,
        'rss:link'        => $link, // FIXME: should go to view URL, not feed URL
        'rss:description' => $description,
        'rss:items'       => $index->uriref(),
      ),
    ));

    // Compile the channel index and render the items:
    $items = array();
    foreach ($this->view->result as $row) {
      $index[] = url('node/'. $row->nid, array('absolute' => TRUE));
      $items = array_merge($items, $this->row_plugin->render($row));
    }

    $data = rdf_normalize(array_merge($channel, $index->to_triples(), $items));

    $filename = implode('.', array('feed', $format->file_ext));
    $content_type = $format->mime_type;
    //$content_type = 'text/plain'; // DEBUG: disable for production.
    $content_encoding = $format->encoding;
    $content = rdf_serialize($data, array('format' => $format->name, 'language' => $language->language));

    drupal_set_header('Content-Disposition: inline; filename='. $filename);
    drupal_set_header('Content-Type: '. $content_type .'; charset='. $content_encoding);
    drupal_set_header('Content-Length: '. strlen($content));
    return $content;
  }
}
