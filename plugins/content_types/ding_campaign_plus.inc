<?php

/**
 * @file
 * Ding Campaign CTools plugin file.
 */

$plugin = array(
  'title' => t('Relevant campaign plus'),
  'description' => t('Display ad-like campaigns'),
  'single' => TRUE,
  'defaults' => array(
    'style' => 'ribbon',
  ),
  'edit form' => 'ding_campaign_plus_relevant_content_type_edit_form',
  'render callback' => 'ding_campaign_plus_relevant_content_type_render',
  'required context' => array(
    new ctools_context_optional(t('Page node'), 'node'),
    new ctools_context_optional(t('Related library'), 'node'),
    new ctools_context_optional(t('Search term'), 'string'),
    new ctools_context_optional(t('Taxonomy term'), array('entity:taxonomy_term', 'taxonomy_term')),
  ),
  'category' => t('Ding!'),
);

/**
 * Implements hook_content_type_render().
 */
function ding_campaign_plus_relevant_content_type_render($subtype, $conf, $panel_args, $context) {
  $block = new stdClass();
  $context_data = array(
    'page' => isset($context[0]->data) ? $context[0]->data : NULL,
    'library' => isset($context[1]->data) ? $context[1]->data : NULL,
    'search_term' => isset($context[2]->data) ? $context[2]->data : NULL,
    'taxonomy_term' => isset($context[3]->data) ? $context[3]->data : NULL,
    'path' => (object) array(
      'raw' => arg(),
      'path' => drupal_get_path_alias($_GET['q']),
    ),
  );
  $count = $conf['ding_campaign_count'];
  $offset = $conf['ding_campaign_offset'];
  $style = !empty($conf['style']) ? $conf['style'] : 'ribbon';

  // Check cache to see if campaign trigger has been calculated for this
  // context.
  $cid = _ding_campaign_plus_cache_id($context_data, $style);
  $cache = null;// cache_get($cid);
  if (isset($cache->data) && REQUEST_TIME < $cache->expire) {
    $campaign_nids = $cache->data;
  }
  else {
    $campaign_nids = ding_campaign_plus_get_matches($context_data, $style,$offset,$count);
    cache_set($cid, $campaign_nids, 'cache', REQUEST_TIME + variable_get('ding_campaign_plus_expire', 900));
  }

  $block->content = '';
  if (!empty($campaign_nids)) {
    $campaign = FALSE;
    foreach ($campaign_nids as $nid) {
      // Use the first campaign only.
      $campaign = node_load($nid);

      // Check if campaign matches style.
      if ($campaign->field_ding_campaign_plus_style[LANGUAGE_NONE][0]['value'] === $style) {
        // Campaign found that matches style.
        
      }
      else {
        $campaign = FALSE;
      }
    
        if ($campaign) {
          $block->content .= drupal_render(node_view($campaign, 'full'));
        }
    }
  }
  if (user_access('access campaign rules')) {
    ctools_include('modal');
    ctools_modal_add_js();
    $modal_link = l(
      t('Select campaigns'),
      'ding_campaign_plus/nojs/select',
      array(
        'query' => drupal_get_destination(),
        'attributes' => array(
          'class' => array('ctools-use-modal'),
          'title' => t('Select campaigns for this path'),
        ),
      )
    );
    $block->content .= '<p>' . $modal_link . '</p>';
  }
  return $block;
}

/**
 * Implements hook_content_type_edit_form().
 */
function ding_campaign_plus_relevant_content_type_edit_form($form, &$form_state) {

  $form['ding_campaign_count'] = array(
    '#type' => 'textfield',
    '#title' => t('Campaign count'),
    '#description' => t('The maximum number of campaigns to display'),
    '#default_value' => (isset($form_state['conf']['ding_campaign_count'])) ? $form_state['conf']['ding_campaign_count'] : 3,
    '#size' => 2,
    '#maxlength' => 2,
  );

  $form['ding_campaign_offset'] = array(
    '#type' => 'textfield',
    '#title' => t('Campaign offset'),
    '#description' => t('Which campaign should we count from when returning campaigns. This is used to avoid displaying the same campaigns when using multiple campaign panes on the same panel. Use 0 to return the most relevant campaigns.'),
    '#default_value' => (isset($form_state['conf']['ding_campaign_offset'])) ? $form_state['conf']['ding_campaign_offset'] : 0,
    '#size' => 3,
    '#maxlength' => 3,
  );
  $form['style'] = array(
    '#type' => 'select',
    '#title' => t('Campaign style'),
    '#description' => t('The display type of the campaigns in this pane.'),
    '#options' => array(
      'ribbon' => t('Ribbon'),
      'box' => t('Box'),
    ),
    '#default_value' => $form_state['conf']['style'],
  );

  return $form;
}

/**
 * Implements hook_content_type_edit_form_validate().
 */
function ding_campaign_plus_relevant_content_type_edit_form_validate($form, &$form_state) {
  return $form;
}

/**
 * Implements hook_content_type_edit_form_submit().
 */
function ding_campaign_plus_relevant_content_type_edit_form_submit($form, &$form_state) {
  $form_state['conf']['ding_campaign_count'] = $form_state['values']['ding_campaign_count'];
  $form_state['conf']['ding_campaign_offset'] = $form_state['values']['ding_campaign_offset'];
  $form_state['conf']['style'] = $form_state['values']['style'];
}
