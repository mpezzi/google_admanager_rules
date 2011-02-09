<?php
// $Id$

/**
 * @file
 * Provides internal functions for google_admanager_rules module.
 */

/**
 * Save a Google Admanager Path Object.
 * 
 */
function google_admanager_rules_save($rule) {
  $rule['is_new'] = empty($rule['rid']);
  
  if ( !$rule['is_new'] ) {
    drupal_write_record('google_admanager_rules', $rule, array('rid'));
  }
  else {
    drupal_write_record('google_admanager_rules', $rule);
  }
  
  cache_clear_all('google_admanager_rules:rules', 'cache');
}

/**
 * Load a Google Admanager Path Object.
 */
function google_admanager_rules_load($rid) {
  static $rules = array();
  
  if ( !isset($rules[$rid]) && is_numeric($rid) ) {
    $result = db_query("SELECT * FROM {google_admanager_rules} WHERE rid = %d", $rid);
    
    if ( $rule = db_fetch_object($result) ) {
      $rules[$rid] = $rule;
    }
    else {
      $rule = FALSE;
    }
  }
  else {
    $rule = FALSE;
  }
  
  return $rule;
}

/**
 * Load all active Google Admanager Path Objects.
 */
function google_admanager_rules_load_active() {
  static $rules;
  
  $cache = cache_get('google_admanager_rules:rules');
  
  if ( !$cache ) {
    if ( !isset($rules) ) {
      $rules = array();
      $result = db_query("SELECT * FROM {google_admanager_rules} WHERE status = 1");
      
      while ( $rule = db_fetch_object($result) ) {
        $rules[$rule->rid] = $rule;
      }
    }
    
    cache_set('google_admanager_rules:rules', $rules, 'cache');
    
    return $rules;
  }
  
  return $cache->data;
}

/**
 * Determine rule for a given path.
 */
function google_admanager_rules_get_rules_by_path($query_path) {
  $matched_rules = array();
  
  foreach ( google_admanager_rules_load_active() as $rule ) {
    $rules = explode("\n", $rule->rule);
    
    foreach ( $rules as $r ) {
      if ( strpos($r, 'term:') !== FALSE && arg(0) == 'node' && is_numeric(arg(1)) ) {
        list($t, $vocab_name, $term_name) = explode(':', $r);
        
        if ( isset($vocab_name) && isset($term_name) ) {
          $node = node_load(arg(1));
          
          // Find vocabulary id.
          foreach ( taxonomy_get_vocabularies($node->type) as $vocab ) {
            if ( strtolower($vocab->name) == strtolower($vocab_name) ) {
              $vid = $vocab->vid;
            }
          }
          
          // If vocabulary found, look for taxonomy term.
          if ( isset($vid) ) {
            foreach ( taxonomy_node_get_terms_by_vocabulary($node, $vid) as $term ) {
              if ( strtolower($term->name) == strtolower($term_name) ) {
                $matched_rules[$rule->rid] = $rule;
              }
            }
          }
          
        }
        
      }
      else {
        $regex = '/^('. 
                  preg_replace(array('/(\r\n?|\n)/', '/\\\\\*/', '/(^|\|)\\\\<front\\\\>($|\|)/'), 
                               array('|', '.*', '\1'. preg_quote(variable_get('site_frontpage', 'node'), '/') .'\2'), 
                               preg_quote($r, '/')) .
                 ')$/';

        $page_match = preg_match($regex, $query_path);

        if ( $query_path != $_GET['q'] ) {
          $page_match = $page_match || preg_match($regex, $_GET['q']);
        }

        if ( $page_match > 0 ) {
          $matched_rules[$rule->rid] = $rule;
        }
      }
    }
  }
  
  return $matched_rules;
}
