<?php


/**
 * @file
 * Views integration for AI Related Content.
 */

/**
 * Implements hook_views_data().
 */
function ai_related_content_views_data() {
  $data = [];

  // Create a new pseudo-field that can be used as a contextual filter
  $data['ai_related_content']['table']['group'] = t('AI Related Content');

  $data['ai_related_content']['table']['join'] = [
    '#global' => [],
  ];

  $data['ai_related_content']['nid'] = [
    'title' => t('AI Related Content'),
    'help' => t('Finds nodes related to the given node ID using AI vector similarity.'),
    'argument' => [
      'handler' => 'ai_related_content_handler_argument',
    ],
  ];

  return $data;
}
