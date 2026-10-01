
# AI Related Content

Displays related nodes using AI embeddings and a vector search.

## Installation

- Install this module using the official
  [Backdrop CMS instructions](https://backdropcms.org/user-guide/modules).

## Popularity (trending) blending

Related results can be re-ranked with recent page views so trending items
among the related candidates rise. Unrelated popular content is never added.

- A small JavaScript beacon on full node pages posts one view per node per
  browser session to `ai-related-content/hit`, so views served from the page
  cache still count. Bots, users with "administer nodes", and IPs over the
  hourly limit are ignored.
- Views are stored as daily counts in `ai_related_content_views` and pruned on
  cron after the configured window.
- Ranking: `blended = (1 - w) * similarity + w * trending`, both normalised to
  0..1 within the candidate pool. Trending uses exponential decay (configurable
  half-life) and `log1p` damping.
- While blending is on, results are cached for the shorter popularity
  cache lifetime instead of one week.

Configure under **Popularity (trending)** at
`admin/config/ai/ai-related-content`. Blending is off by default: switch on
"Blend popularity into related content ranking" to use it. View tracking is a
separate setting, so history can build up before blending is switched on.
Turning blending off restores pure similarity ranking. The test tab shows
similarity, trending views and blended score.

Note: the Views block narrows results to the chosen node IDs, then applies the
View's own sort. Blending changes which items appear there; the legacy block
also keeps the blended order.

## Issues

Bugs and feature requests should be reported in the
[Issue Queue](https://github.com/backdrop-contrib/ai_related_content/issues).

## Current Maintainer

[Justin Keiser](https://github.com/keiserjb)

## Credits

- Created for Backdrop CMS by [Justin Keiser](https://github.com/keiserjb)
- Inspired by the Drupal module of the same name.
- Developed with AI assistance.

## License

This project is GPL v2 software. See the LICENSE.txt file in this directory for complete text.

