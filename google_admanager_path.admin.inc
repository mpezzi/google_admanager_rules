<?php
// $Id$

/**
 * @file
 * Google Admanager Path page callback file for google_admanager_path module.
 */

/**
 * Administration Pages.
 */
function google_admanager_path_admin($callback = '') {
  $op = isset($_POST['op']) ? $_POST['op'] : $callback;
  
  if ( !empty($_POST['paths']) && isset($_POST['operation']) && ( $_POST['operation'] == 'delete' ) ) {
    $output = drupal_get_form('google_admanager_path_multiple_delete_confirm');
  }
  else {
    $output = drupal_get_form('google_admanager_path_admin_form');
  }
  
  return $output;
}

/**
 * Form builder; Return form for advertisement administration form.
 */
function google_admanager_path_admin_form() {
  $form = array();
  
  drupal_add_css(drupal_get_path('module', 'google_admanager_path') . '/google_admanager_path.css');
  
  // Headers.
  $header = array(
    array(),
    array('data' => t('Tag'), 'field' => 'p.tag', 'sort' => 'asc'),
    array('data' => t('Name'), 'field' => 'p.name'),
    array('data' => t('Rule'), 'field' => 'p.rule'),
    array('data' => t('Status'), 'field' => 'p.status'),
    t('Operations'),
  );
  
  // Query.
  $sql = "SELECT p.pid, p.tag, p.name, p.status, p.rule FROM {google_admanager_path} as p";
  $sql .= tablesort_sql($header);
  $sql_count = "SELECT COUNT(p.pid) FROM {google_admanager_path} as p";

  $result = pager_query($sql, 50, 0, $sql_count);

  $status = array(t('Disabled'), t('Enabled'));

  $destination = drupal_get_destination();

  $paths = array();
  while ( $path = db_fetch_object($result) ) {
    $rules = $path->rule ? explode("\n", $path->rule) : array();
    $rules = array_map('check_plain', $rules);
    
    $paths[$path->pid] = '';
    $form['tag'][$path->pid] = array('#value' => $path->tag);
    $form['name'][$path->pid] = array('#value' => $path->name);
    $form['rule'][$path->pid] = array('#value' => theme('item_list', $rules));
    $form['status'][$path->pid] = array('#value' => $status[$path->status]);
    $form['operations'][$path->pid] = array('#value' => l('Edit', 'admin/content/advertisement/edit/' . $path->pid, array('query' => $destination)));
  }
  
  $form['operation'] = array(
    '#type' => 'hidden',
    '#value' => 'delete',
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Delete'),
  );

  $form['paths'] = array(
    '#type' => 'checkboxes',
    '#options' => $paths,
  );

  $form['pager'] = array(
    '#value' => theme('pager', NULL, 50, 0),
  );
  
  return $form;
}

/**
 * Form builder; Confirm multiple delete.
 */
function google_admanager_path_multiple_delete_confirm(&$form_state) {
  $edit = $form_state['post'];
  
  $form['paths'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );
  
  foreach ( $edit['paths'] as $pid => $value ) {
    if ( $value != 0 ) {
      $path = google_admanager_path_load($pid);
      
      $form['paths'][$pid] = array(
        '#type' => 'hidden',
        '#value' => $pid,
        '#prefix' => '<li>',
        '#suffix' => check_plain($path->tag . ' -> ' . $path->name) . '</li>',
      );
    }
  }
  
  $form['operation'] = array(
    '#type' => 'hidden',
    '#value' => 'delete',
  );
  
  return confirm_form($form, 
          t('Are you sure you want to delete these paths?'), 
          'admin/content/advertisement', 
          t('This action cannot be undone.'), 
          t('Delete all'), 
          t('Cancel')
        );
}

/**
 * Form builder; Submit confirm multiple delete.
 */
function google_admanager_path_multiple_delete_confirm_submit($form, &$form_state) {
  $i = 0;
  foreach ( $form_state['values']['paths'] as $pid ) {
    if ( $pid != 0 ) {
      db_query("DELETE FROM {google_admanager_path} WHERE pid = %d", $pid);
      $i++;
    }
  }

  drupal_set_message($i . ' paths have been deleted');

  $form_state['redirect'] = 'admin/content/advertisement';
}

/**
 * Form Builder; Path Object form.
 */
function google_admanager_path_form($form, $path = NULL) {
  $form = array();
  
  $form['pid'] = array(
    '#type' => 'hidden',
    '#value' => isset($path->pid) ? $path->pid : '',
  );
  
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('The name of the custom critera rule.<br />Example: design, food, sponsor_name, etc.'),
    '#default_value' => isset($path->name) ? $path->name : '',
  );
  
  $form['tag'] = array(
    '#type' => 'textfield',
    '#title' => t('Tag'),
    '#description' => t('The group or tag name of the custom critera rule.<br />Example: sponsor, section, etc.'),
    '#default_value' => isset($path->tag) ? $path->tag : '',
  );
  
  $form['rule'] = array(
    '#type' => 'textarea',
    '#title' => t('Rule'),
    '#description' => t('Paths or terms that will activate this critera rule. <strong>One per line</strong>.<br />Example: node/180, design/article, term:sponsors:company, etc.'),
    '#cols' => 20,
    '#rows' => 10,
    '#default_value' => isset($path->rule) ? $path->rule : '',
  );
  
  $form['status'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enabled'),
    '#default_value' => isset($path->status) ? $path->status : 0,
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
function google_admanager_path_form_submit($form, &$form_state) {
  $path = $form_state['values'];
  
  google_admanager_path_save($path);
  
  drupal_set_message('The Advertisement path has been saved.');
}

/**
 * Theme path administration overview.
 */
function theme_google_admanager_path_admin_form($form) {
  
  // Headers.
  $header = array(
    theme('table_select_header_cell'),
    array('data' => t('Tag'), 'field' => 'p.tag'),
    array('data' => t('Name'), 'field' => 'p.name'),
    array('data' => t('Rule'), 'field' => 'p.rule'),
    array('data' => t('Status'), 'field' => 'p.status'),
    t('Operations'),
  );
  
  if ( isset($form['tag']) && is_array($form['name']) ) {
    foreach ( element_children($form['name']) as $key ) {
      $rows[] = array(
        drupal_render($form['paths'][$key]),
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
