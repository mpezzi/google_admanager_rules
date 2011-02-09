<?php
// $Id$

/**
 * @file
 * Provides internal functions for google_admanager_path module.
 */

/**
 * Save a Google Admanager Path Object.
 * 
 */
function google_admanager_path_save($path) {
  $path['is_new'] = empty($path['pid']);
  
  if ( !$path['is_new'] ) {
    drupal_write_record('google_admanager_path', $path, array('pid'));
  }
  else {
    drupal_write_record('google_admanager_path', $path);
  }
  
  cache_clear_all('google_admanager_path:paths', 'cache');
}

/**
 * Load a Google Admanager Path Object.
 */
function google_admanager_path_load($pid) {
  static $paths = array();
  
  if ( !isset($paths[$pid]) && is_numeric($pid) ) {
    $result = db_query("SELECT * FROM {google_admanager_path} WHERE pid = %d", $pid);
    
    if ( $path = db_fetch_object($result) ) {
      $paths[$pid] = $path;
    }
    else {
      $path = FALSE;
    }
  }
  else {
    $path = FALSE;
  }
  
  return $path;
}

/**
 * Load all active Google Admanager Path Objects.
 */
function google_admanager_path_load_active() {
  static $paths;
  
  $cache = cache_get('google_admanager_path:paths');
  
  if ( !$cache ) {
    if ( !isset($paths) ) {
      $paths = array();
      $result = db_query("SELECT * FROM {google_admanager_path} WHERE status = 1");
      
      while ( $path = db_fetch_object($result) ) {
        $paths[$path->pid] = $path;
      }
    }
    
    cache_set('google_admanager_path:paths', $paths, 'cache');
    
    return $paths;
  }
  
  return $cache->data;
}

/**
 * Determine rule for a given path.
 */
function google_admanager_path_get_paths_by_rule($query_path) {
  $matched_paths = array();
  
  foreach ( google_admanager_path_load_active() as $path ) {
    $rules = explode("\n", $path->rule);
    
    foreach ( $rules as $rule ) {
      if ( strpos($rule, 'term:') !== FALSE && arg(0) == 'node' && is_numeric(arg(1)) ) {
        list($t, $vocab_name, $term_name) = explode(':', $rule);
        
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
                $matched_paths[$path->pid] = $path;
              }
            }
          }
          
        }
        
      }
      else {
        $regex = '/^('. 
                  preg_replace(array('/(\r\n?|\n)/', '/\\\\\*/', '/(^|\|)\\\\<front\\\\>($|\|)/'), 
                               array('|', '.*', '\1'. preg_quote(variable_get('site_frontpage', 'node'), '/') .'\2'), 
                               preg_quote($rule, '/')) .
                 ')$/';

        $page_match = preg_match($regex, $query_path);

        if ( $query_path != $_GET['q'] ) {
          $page_match = $page_match || preg_match($regex, $_GET['q']);
        }

        if ( $page_match > 0 ) {
          $matched_paths[$path->pid] = $path;
        }
      }
    }
  }
  
  return $matched_paths;
}
