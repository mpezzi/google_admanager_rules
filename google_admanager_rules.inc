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
function google_admanager_rules_get_rules_by_path() {
  $matched_rules = array();
  
  // Include both the path alias and normal path matching.
  $current_path = array(drupal_get_path_alias($_GET['q']));
  if ( $current_path[0] != $_GET['q'] ) {
    $current_path[] = $_GET['q'];
  }
  
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
        if ( google_admanager_rules_match($current_path, $r, TRUE) ) {
          $matched_rules[$rule->rid] = $rule;
        }
      }
    }
  }
  
  return $matched_rules;
}

/**
 * Determine if the subject matches the patterns path.
 * - Code modified slightly from context module.
 */
function google_admanager_rules_match($subject, $pattern, $path = FALSE) {
  static $regexps;
  $match = FALSE;
  $positives = $negatives = 0;
  $subject = !is_array($subject) ? array($subject) : $subject;
  
  if (strpos($pattern, '~') === 0) {
    $negate = TRUE;
    $negatives++;
  }
  else {
    $negate = FALSE;
    $positives++;
  }
  $pattern = ltrim($pattern, '~');
  if (!isset($regexps[$pattern])) {
    if ($path) {
      $regexps[$pattern] = '/^('. preg_replace(array('/(\r\n?|\n)/', '/\\\\\*/', '/(^|\|)\\\\<front\\\\>($|\|)/'), array('|', '.*', '\1'. preg_quote(variable_get('site_frontpage', 'node'), '/') .'\2'), preg_quote($pattern, '/')) .')$/';
    }
    else {
      $regexps[$pattern] = '/^('. preg_replace(array('/(\r\n?|\n)/', '/\\\\\*/'), array('|', '.*'), preg_quote($pattern, '/')) .')$/';
    }
  }
  foreach ($subject as $value) {
    if (preg_match($regexps[$pattern], $value)) {
      if ($negate) {
        return FALSE;
      }
      $match = TRUE;
    }
  }

  // If there are **only** negative conditions and we've gotten this far none
  // we actually have a match.
  if ($positives === 0 && $negatives) {
    return TRUE;
  }
  return $match;
}
