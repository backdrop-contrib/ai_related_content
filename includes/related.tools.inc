<?php

/**
 * @file
 * Tool definitions and execution callbacks for ai_related_content agent tools.
 */

/**
 * Returns tool definitions for hook_ai_tools().
 */
function ai_related_content_tools_definitions() {
  $tools = [];

  $tools['get_related_content'] = [
    'definition' => [
      'type' => 'function',
      'function' => [
        'name' => 'get_related_content',
        'description' => 'Retrieves AI-powered semantically related content for a given node or text prompt using vector similarity across configured backends (MariaDB Vector, pgvector, Milvus, Pinecone).',
        'parameters' => [
          'type' => 'object',
          'properties' => [
            'nid' => [
              'type' => 'integer',
              'description' => 'Node ID to find semantically related content for.',
            ],
            'text' => [
              'type' => 'string',
              'description' => 'Optional custom query text to find semantically related content for (used when nid is omitted).',
            ],
            'limit' => [
              'type' => 'integer',
              'description' => 'Maximum number of related items to return (default 3, max 20).',
            ],
            'index_id' => [
              'type' => 'string',
              'description' => 'Optional Search API index machine name to query. Uses default configured index if omitted.',
            ],
          ],
          'additionalProperties' => FALSE,
        ],
      ],
    ],
    'execute' => 'ai_related_content_tool_get_related',
    'operation' => 'read',
    'permission' => 'access content',
    'module' => 'ai_related_content',
    'category' => 'content',
  ];

  $tools['get_related_content_exclusions'] = [
    'definition' => [
      'type' => 'function',
      'function' => [
        'name' => 'get_related_content_exclusions',
        'description' => 'Retrieves the list of nodes and URLs/paths excluded from appearing in AI related content recommendations.',
        'parameters' => [
          'type' => 'object',
          'properties' => [
            'limit' => [
              'type' => 'integer',
              'description' => 'Maximum number of excluded nodes to return (default 50, max 200).',
            ],
          ],
          'additionalProperties' => FALSE,
        ],
      ],
    ],
    'execute' => 'ai_related_content_tool_get_exclusions',
    'operation' => 'read',
    'permission' => 'administer site configuration',
    'module' => 'ai_related_content',
    'category' => 'content',
  ];

  $tools['set_node_related_content_exclusion'] = [
    'definition' => [
      'type' => 'function',
      'function' => [
        'name' => 'set_node_related_content_exclusion',
        'description' => 'Sets or removes the exclusion flag for a specific node to prevent or allow it appearing in related content recommendations.',
        'parameters' => [
          'type' => 'object',
          'properties' => [
            'nid' => [
              'type' => 'integer',
              'description' => 'Node ID to update.',
            ],
            'excluded' => [
              'type' => 'boolean',
              'description' => 'TRUE to exclude node from related recommendations, FALSE to allow.',
            ],
          ],
          'required' => ['nid', 'excluded'],
          'additionalProperties' => FALSE,
        ],
      ],
    ],
    'execute' => 'ai_related_content_tool_set_exclusion',
    'operation' => 'write',
    'permission' => 'administer site configuration',
    'module' => 'ai_related_content',
    'category' => 'content',
  ];

  return $tools;
}

/**
 * Tool execution callback: get_related_content.
 */
function ai_related_content_tool_get_related(array $parameters = []) {
  $nid = isset($parameters['nid']) ? (int) $parameters['nid'] : 0;
  $text = isset($parameters['text']) ? trim((string) $parameters['text']) : '';
  $limit = isset($parameters['limit']) ? max(1, min((int) $parameters['limit'], 20)) : 3;
  $index_id = !empty($parameters['index_id']) ? trim((string) $parameters['index_id']) : NULL;

  if ($nid <= 0 && $text === '') {
    return [
      'error' => 'Either "nid" or "text" must be provided to find related content.',
    ];
  }

  $related = [];
  $source_info = [];

  if ($nid > 0) {
    $node = node_load($nid);
    if (!$node) {
      return ['error' => "Node $nid not found."];
    }
    if (!node_access('view', $node)) {
      return ['error' => "Access denied to node $nid."];
    }
    $source_info = [
      'nid' => (int) $node->nid,
      'title' => $node->title,
      'type' => $node->type,
    ];
    $related = ai_related_content_get_related_matches($node, $limit, $index_id);
  }
  else {
    $embedding = ai_related_content_get_embedding($text, $index_id);
    if (empty($embedding)) {
      return ['error' => 'Failed to generate embedding vector for the provided text.'];
    }
    $related = ai_related_content_vector_search($embedding, NULL, $limit, $index_id);
    $source_info = [
      'query_text' => $text,
    ];
  }

  $items = [];
  foreach ($related as $match) {
    $match_nid = (int) $match['nid'];
    $rel_node = node_load($match_nid);
    $title = $rel_node ? $rel_node->title : ($match['title'] ?? '');
    $score = isset($match['score']) ? (float) $match['score'] : NULL;

    $item = [
      'nid' => $match_nid,
      'title' => $title,
      'url' => url('node/' . $match_nid, ['absolute' => TRUE]),
    ];
    if ($score !== NULL) {
      $item['score'] = round($score, 4);
    }
    if ($rel_node) {
      $item['type'] = $rel_node->type;
      $item['created'] = format_date($rel_node->created, 'short');
    }
    $items[] = $item;
  }

  return [
    'source' => $source_info,
    'count' => count($items),
    'items' => $items,
  ];
}

/**
 * Tool execution callback: get_related_content_exclusions.
 */
function ai_related_content_tool_get_exclusions(array $parameters = []) {
  $limit = isset($parameters['limit']) ? max(1, min((int) $parameters['limit'], 200)) : 50;

  $excluded_nodes = [];
  if (db_table_exists('ai_related_content_exclude')) {
    $nids = db_select('ai_related_content_exclude', 'e')
      ->fields('e', ['nid'])
      ->range(0, $limit)
      ->execute()
      ->fetchCol();

    foreach ($nids as $nid) {
      $node = node_load($nid);
      $excluded_nodes[] = [
        'nid' => (int) $nid,
        'title' => $node ? $node->title : t('(deleted or not found)'),
      ];
    }
  }

  $config = config('ai_related_content.settings');
  $excluded_urls_raw = $config->get('excluded_urls') ?: '';
  $excluded_urls = array_values(array_filter(array_map('trim', explode("\n", $excluded_urls_raw))));

  return [
    'count_excluded_nodes' => count($excluded_nodes),
    'excluded_nodes' => $excluded_nodes,
    'count_excluded_urls' => count($excluded_urls),
    'excluded_urls' => $excluded_urls,
  ];
}

/**
 * Tool execution callback: set_node_related_content_exclusion.
 */
function ai_related_content_tool_set_exclusion(array $parameters) {
  $nid = isset($parameters['nid']) ? (int) $parameters['nid'] : 0;
  if ($nid <= 0) {
    return ['error' => 'A valid positive integer nid is required.'];
  }

  $node = node_load($nid);
  if (!$node) {
    return ['error' => "Node $nid not found."];
  }

  $excluded = !empty($parameters['excluded']);
  ai_related_content_set_excluded($nid, $excluded);
  ai_related_content_invalidate_caches($nid);

  return [
    'status' => TRUE,
    'nid' => $nid,
    'title' => $node->title,
    'excluded' => $excluded,
    'message' => $excluded
      ? "Node $nid ($node->title) is now excluded from related content recommendations."
      : "Node $nid ($node->title) exclusion was removed.",
  ];
}
