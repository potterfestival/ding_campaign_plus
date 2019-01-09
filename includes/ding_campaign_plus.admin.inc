<?php

/**
 * @file
 * Defines the administration UI for the module.
 */

/**
 * Administration settings form for the module.
 */
function ding_campaign_plus_admin_settings($form, $form_state) {
  $form = array(
    '#theme' => 'ding_campaign_plus_admin_form',
    '#tree' => TRUE,
  );

  $form['ding_campaign_plus_weight'] = array(
    '#type' => 'fieldset',
    '#title' => t('Weight'),
    '#description' => t('Set the weight for the different triggers. If more campaigns are triggered by the same triggers these weights are used to determined which should be displayed.'),
    '#weight' => -10,
  );

  // Build table options sorted by selected weight.
  $defaults = variable_get('ding_campaign_plus_weight', array());
  $options = ding_campaign_plus_ding_settings_options($defaults);
  foreach ($options as $key => $option) {
    $form['ding_campaign_plus_weight']['table'][$key] = ding_campaign_plus_admin_settings_form_add($option['title'], $option['weight']);
  }

  // Time interval for scanning campaigns for 0-hits.
  $period = drupal_map_assoc(array(
    0,
    3600,
    10800,
    21600,
    32400,
    43200,
    86400,
    172800,
    345600,
    604800,
    1209600,
    2419200,
  ), 'format_interval');

  $form['ding_campaign_plus_notification'] = array(
    '#type' => 'select',
    '#title' => t('Notification scan interval'),
    '#description' => t('Cached pages will not be re-created until at least this much time has elapsed.'),
    '#options' => $period,
    '#default_value' => variable_get('ding_campaign_plus_notification', 604800),
    '#weight' => 12,
  );

  $form['ding_campaign_plus_expire'] = array(
    '#type' => 'textfield',
    '#title' => t('Campaign cache expire'),
    '#description' => t('Campaign selection is cached to boost performance. This is the number of seconds before campaigns should be reselected base on triggers for a give context.'),
    '#default_value' => variable_get('ding_campaign_plus_expire', 900),
    '#size' => 8,
    '#weight' => 15,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save Changes'));

  return $form;
}

/**
 * Submit handler for the administration settings form.
 */
function ding_campaign_plus_admin_settings_submit($form, $form_state) {
  $weights = array();
  foreach ($form_state['values']['ding_campaign_plus_weight']['table'] as $key => $value) {
    $weights[$key] = $value['weight'];
  }

  variable_set('ding_campaign_plus_weight', $weights);
  variable_set('ding_campaign_plus_expire', $form_state['values']['ding_campaign_plus_expire']);
}

/**
 * Build options (table rows) for the administration UI.
 *
 * @param array $weights
 *   The weights used to determine the sort order of the options.
 *
 * @return array
 *   The options sorted by weight.
 */
function ding_campaign_plus_ding_settings_options(array $weights) {
  $options = array();

  foreach (module_implements('ding_campaign_plus_default_weights') as $module) {
    $types = module_invoke($module, 'ding_campaign_plus_default_weights');
    foreach ($types as $type => $info) {
      $options[$type] = array(
        'title' => $info['prefix'] . ' - ' . $info['title'],
        'weight' => isset($weights[$type]) ? $weights[$type] : $info['weight'],
      );
    }
  }

  $options += array(
    'search' => array(
      'title' => 'Search (CQL)',
      'weight' => isset($weights['object']) ? $weights['object'] : 0,
    ),
  );

  // Sort options be weight keeping the array keys.
  uasort($options, '_ding_campaign_plus_ding_settings_options_sort');

  return $options;
}

/**
 * Compare function use to sort options.
 *
 * @param array $a
 *   Single option.
 * @param array $b
 *   Single option.
 *
 * @return int
 *   The result of the comparison.
 */
function _ding_campaign_plus_ding_settings_options_sort(array $a, array $b) {
  if ($a['weight'] === $b['weight']) {
    return 0;
  }
  return ($a['weight'] < $b['weight']) ? -1 : 1;
}

/**
 * Build single row for use in the table.
 *
 * @param string $title
 *   The title to display in the table.
 * @param int $weight
 *   The weight of the row in the table.
 *
 * @return array
 *   Form element for the table.
 */
function ding_campaign_plus_admin_settings_form_add($title, $weight = 0) {
  return array(
    // We'll use a form element of type '#markup' to display the item name.
    'name' => array(
      '#markup' => check_plain($title),
    ),

    'weight' => array(
      '#type' => 'weight',
      '#title' => t('Weight'),
      '#default_value' => $weight,
      '#delta' => 10,
      '#title_display' => 'invisible',
    ),
  );
}

/**
 * Implements theme_ding_campaign_plus_admin_form().
 *
 * Themes the admin forms weight table.
 */
function theme_ding_campaign_plus_admin_form($variables) {
  $form = $variables['form'];

  $rows = array();
  foreach (element_children($form['ding_campaign_plus_weight']['table']) as $id) {
    $form['ding_campaign_plus_weight']['table'][$id]['weight']['#attributes']['class'] = array('campaign-type-weight');
    $rows[] = array(
      'data' => array(
        drupal_render($form['ding_campaign_plus_weight']['table'][$id]['name']),
        drupal_render($form['ding_campaign_plus_weight']['table'][$id]['weight']),
      ),
      'class' => array('draggable'),
    );
  }

  $header = array(t('Name'), t('Weight'));
  $table_id = 'campaign-types-table';

  $output = theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'attributes' => array('id' => $table_id),
  ));

  $form['ding_campaign_plus_weight']['table'] = array(
    '#type' => 'markup',
    '#markup' => $output,
  );

  $output = drupal_render_children($form);

  drupal_add_tabledrag($table_id, 'order', 'sibling', 'campaign-type-weight');

  return $output;
}
/**
 * Ajax callback for selecting campaign on the current page.
 *
 * @param $type
 *   Type.
 * @param $input
 *   Input.
 */
function ding_campaign_plus_admin_select($js = NULL, $step = NULL) 
{
  if (!$js) {
    return drupal_get_form('ding_campaign_admin_select_form');
  }
  ctools_include('modal');
  ctools_include('ajax');
  $form_state = array(
    'title' => t('Select campaigns'),
    'ajax' => TRUE,
  );
  $output = ctools_modal_form_wrapper('ding_campaign_plus_admin_select_form', $form_state);
  print ajax_render($output);
  exit;
    
}
/*
 * frontend admin form for selecting active rules pr. page.
 */
function ding_campaign_plus_admin_select_form($form, &$form_state) {
    
    $destination = drupal_get_destination();
    $split_destination = parse_url($destination['destination']);
    $path = $split_destination['path'];

    $path_alias = drupal_get_path_alias($path);
    $front_page = variable_get('site_frontpage', 'node');
    if ($path == $front_page) {
      $path_alias = '<front>';
    }
    $path_arr = explode('/', $path);
    $context_data = array(
    'page' => node_load($path_arr[1],null,true),
    'library' => isset($context[1]->data) ? $context[1]->data : NULL,
    'search_term' => isset($context[2]->data) ? $context[2]->data : NULL,
    'taxonomy_term' => isset($context[3]->data) ? $context[3]->data : NULL,
    'path' => (object) array(
      'raw' => $destination,
      'path' => $path,
    ),
  );
  $conf = null;
  $count = 300; //make sure we can include all the needed campaigns.
  $offset = 0;
  $style = !empty($conf['style']) ? $conf['style'] : 'ribbon';
  /*
   * Get all campaigns that can match.
   * At first we only support path based rules , as we are missing the panel context.
   */
  $defaults = array();
  
  $defaults = ding_campaign_plus_get_matches($context_data, $style,$offset,$count);
  $options = ding_campaign_plus_admin_get_list();
  $form['path'] = array(
    '#type' => 'hidden',
    '#default_value' => $path,
  );
  $form['path_alias'] = array(
    '#type' => 'hidden',
    '#default_value' => $path_alias,
  );
  $form['ding_campaigns_wrap'] = array(
    '#type' => 'fieldset',
    '#title' => 'Available campaigns',
  );
  $form['ding_campaigns_wrap']['ding_campaigns'] = array(
    '#type' => 'checkboxes',
    '#default_value' => $defaults,
    '#options' => $options,
    '#description' => 'Select campaigns asociated with this node.',
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  return $form;
}
/*
 * Submit form for ajax frontend selection tool.
 */
function ding_campaign_plus_admin_select_form_submit($form, &$form_state)
{
  $destination = drupal_get_destination();
  $path = $form_state['values']['path'];
  $path_arr = explode('/', $path);
  $path_alias = $form_state['values']['path_alias'];
  $defaults = $form['ding_campaigns_wrap']['ding_campaigns']['#default_value'];
  // Throw away all disabled campaigns.
  $campaigns = array_filter($form_state['input']['ding_campaigns']);
  
  $added = $campaigns;
  $removed = array_diff_key($form['ding_campaigns_wrap']['ding_campaigns']['#options'],$added);
  foreach ($added as $campaign) {
      switch(ding_campaign_plus_admin_get_type($campaign)){
          case'page':
          case'path':
          {
              $node = node_load($campaign);
              if(isset($node->tabs[DING_CAMPAIGN_PLUS_BASIC_TYPE]))
              {
                  $node->tabs[DING_CAMPAIGN_PLUS_BASIC_TYPE]['rules'][] = array('type' => 'path' , 'value' =>  $path);
              }
              else
              {
                $node->tabs[DING_CAMPAIGN_PLUS_BASIC_TYPE] = [];
                $node->tabs[DING_CAMPAIGN_PLUS_BASIC_TYPE]['rules'] = array();
                $node->tabs[DING_CAMPAIGN_PLUS_BASIC_TYPE]['rules'][] = array('type' => 'path' , 'value' =>  $path);
              }
              ding_campaign_plus_basic_node_update($node);
              break;
          }
          default:
          {
              $node = node_load($campaign);
              $node->tabs[DING_CAMPAIGN_PLUS_BASIC_TYPE] = [];
              $node->tabs[DING_CAMPAIGN_PLUS_BASIC_TYPE]['rules'] = array();
              $node->tabs[DING_CAMPAIGN_PLUS_BASIC_TYPE]['rules'][] = array('type' => 'path' , 'value' => $path);
              ding_campaign_plus_basic_node_update($node);
          }
      }
  }
  foreach ($removed as $key => $campaign) {
      switch(ding_campaign_plus_admin_get_type($key)){
          case'page':
          case'path':
          {
              ding_campaign_plus_basic_node_delete(node_load($key));
          }
      }
  }
  if (isset($form_state['ajax']) && $form_state['ajax']) {
    ctools_include('ajax');
    $options = drupal_parse_url($destination['destination']);
    $commands[] = ctools_ajax_command_redirect($options['path'], 0, $options);
    print ajax_render($commands);
    exit;
  }
}
/**
 * Fetch available campaigns.
 *
 * @return
 *   Array of campaigns with ids and titles.
 */
function ding_campaign_plus_admin_get_list() {
  $result = db_query("SELECT n.nid, n.title FROM {node} n
    WHERE n.status <> 0
      AND n.type = 'ding_campaign_plus'
    ORDER BY n.title")
    ->fetchAll();

  $campaigns = array();

  foreach ($result as $k => $v) {
    $campaigns[$v->nid] = $v->title;
  }

  return $campaigns;
}
/*
 * Get campaign types from db 
 */
 function ding_campaign_plus_admin_get_type($cid)
 {
     $query = db_select('ding_campaign_plus_basic', 'dcpb')
    ->fields('dcpb', array('type'))
    ->condition('nid', $cid)->execute();
     
    $result = $query->fetchObject();
    return $result->type; 
 }