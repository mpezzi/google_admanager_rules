<?php
// $Id$

/**
 * @file
 * Google Admanager Rules page callback file for google_admanager_rules module.
 */

/**
 * Administration Pages.
 */
function google_admanager_rules_admin($callback = '') {
  $op = isset($_POST['op']) ? $_POST['op'] : $callback;
  
  if ( !empty($_POST['rules']) && isset($_POST['operation']) && ( $_POST['operation'] == 'delete' ) ) {
    $output = drupal_get_form('google_admanager_rules_multiple_delete_confirm');
  }
  else {
    $output = drupal_get_form('google_admanager_rules_admin_form');
  }
  
  return $output;
}

/**
 * Form builder; Return form for advertisement administration form.
 */
function google_admanager_rules_admin_form() {
  $form = array();
  
  drupal_add_css(drupal_get_path('module', 'google_admanager_rules') . '/google_admanager_rules.css');
  
  // Headers.
  $header = array(
    array(),
    array('data' => t('Tag'), 'field' => 'r.tag', 'sort' => 'asc'),
    array('data' => t('Name'), 'field' => 'r.name'),
    array('data' => t('Rules'), 'field' => 'r.rule'),
    array('data' => t('Status'), 'field' => 'r.status'),
    t('Operations'),
  );
  
  // Query.
  $sql = "SELECT r.rid, r.tag, r.name, r.status, r.rule FROM {google_admanager_rules} as r";
  $sql .= tablesort_sql($header);
  $sql_count = "SELECT COUNT(r.rid) FROM {google_admanager_rules} as r";

  $result = pager_query($sql, 50, 0, $sql_count);

  $status = array(t('Disabled'), t('Enabled'));

  $destination = drupal_get_destination();

  $rules = array();
  while ( $rule = db_fetch_object($result) ) {
    $rules_array = $rule->rule ? explode("\n", $rule->rule) : array();
    $rules_array = array_map('check_plain', $rules_array);
    
    $rules[$rule->rid] = '';
    $form['tag'][$rule->rid] = array('#value' => $rule->tag);
    $form['name'][$rule->rid] = array('#value' => $rule->name);
    $form['rule'][$rule->rid] = array('#value' => theme('item_list', $rules_array));
    $form['status'][$rule->rid] = array('#value' => $status[$rule->status]);
    $form['operations'][$rule->rid] = array('#value' => l('Edit', 'admin/content/advertisement/edit/' . $rule->rid, array('query' => $destination)));
  }
  
  $form['operation'] = array(
    '#type' => 'hidden',
    '#value' => 'delete',
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Delete'),
  );

  $form['rules'] = array(
    '#type' => 'checkboxes',
    '#options' => $rules,
  );

  $form['pager'] = array(
    '#value' => theme('pager', NULL, 50, 0),
  );
  
  return $form;
}

/**
 * Form builder; Confirm multiple delete.
 */
function google_admanager_rules_multiple_delete_confirm(&$form_state) {
  $edit = $form_state['post'];
  
  $form['rules'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );
  
  foreach ( $edit['rules'] as $rid => $value ) {
    if ( $value != 0 ) {
      $rule = google_admanager_rules_load($rid);
      
      $form['rules'][$rid] = array(
        '#type' => 'hidden',
        '#value' => $rid,
        '#prefix' => '<li>',
        '#suffix' => check_plain($rule->tag . ' -> ' . $rule->name) . '</li>',
      );
    }
  }
  
  $form['operation'] = array(
    '#type' => 'hidden',
    '#value' => 'delete',
  );
  
  return confirm_form($form, 
          t('Are you sure you want to delete these rules?'), 
          'admin/content/advertisement', 
          t('This action cannot be undone.'), 
          t('Delete all'), 
          t('Cancel')
        );
}

/**
 * Form builder; Submit confirm multiple delete.
 */
function google_admanager_rules_multiple_delete_confirm_submit($form, &$form_state) {
  $i = 0;
  foreach ( $form_state['values']['rules'] as $rid ) {
    if ( $rid != 0 ) {
      db_query("DELETE FROM {google_admanager_rules} WHERE rid = %d", $rid);
      $i++;
    }
  }

  drupal_set_message($i . ' rules have been deleted');

  $form_state['redirect'] = 'admin/content/advertisement';
}

/**
 * Form Builder; Path Object form.
 */
function google_admanager_rules_form($form, $rule = NULL) {
  $form = array();
  
  $form['rid'] = array(
    '#type' => 'hidden',
    '#value' => isset($rule->rid) ? $rule->rid : '',
  );
  
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('The name of the custom critera rule.<br />Example: design, food, sponsor_name, etc.'),
    '#required' => TRUE,
    '#default_value' => isset($rule->name) ? $rule->name : '',
  );
  
  $form['tag'] = array(
    '#type' => 'textfield',
    '#title' => t('Tag'),
    '#description' => t('The group or tag name of the custom critera rule.<br />Example: sponsor, section, etc.'),
    '#required' => TRUE,
    '#default_value' => isset($rule->tag) ? $rule->tag : '',
  );
  
  $form['rule'] = array(
    '#type' => 'textarea',
    '#title' => t('Rules'),
    '#description' => t('Paths or terms that will activate this critera rule. <strong>One per line</strong>.<br />Example: node/180, design/article, term:sponsors:company, etc.'),
    '#cols' => 20,
    '#rows' => 10,
    '#default_value' => isset($rule->rule) ? $rule->rule : '',
  );
  
  $form['status'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enabled'),
    '#default_value' => isset($rule->status) ? $rule->status : 0,
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  
  return $form;
}

/**
 * Form; Submit Path Object.
 */
function google_admanager_rules_form_submit($form, &$form_state) {
  google_admanager_rules_save($form_state['values']);
  
  $form_state['redirect'] = 'admin/content/advertisement';
  
  drupal_set_message('The Advertisement path has been saved.');
}

/**
 * Theme path administration overview.
 */
function theme_google_admanager_rules_admin_form($form) {
  
  // Headers.
  $header = array(
    theme('table_select_header_cell'),
    array('data' => t('Tag'), 'field' => 'r.tag'),
    array('data' => t('Name'), 'field' => 'r.name'),
    array('data' => t('Rules'), 'field' => 'r.rule'),
    array('data' => t('Status'), 'field' => 'r.status'),
    t('Operations'),
  );
  
  if ( isset($form['tag']) && is_array($form['name']) ) {
    foreach ( element_children($form['name']) as $key ) {
      $rows[] = array(
        drupal_render($form['rules'][$key]),
        drupal_render($form['tag'][$key]),
        drupal_render($form['name'][$key]),
        drupal_render($form['rule'][$key]),
        drupal_render($form['status'][$key]),
        drupal_render($form['operations'][$key]),
      );
    }
  }
  else {
    $rows[] = array(array('data' => t('No paths available.'), 'colspan' => '6'));
  }
  
  $output .= theme('table', $header, $rows);
  if ( $form['pager']['#value'] ) {
    $output .= drupal_render($form['pager']);
  }
  
  $output .= drupal_render($form);
  
  return $output;
}
