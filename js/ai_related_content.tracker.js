/**
 * @file
 * Sends one page-view beacon per node per browser session.
 *
 * Runs client-side so views served from the anonymous page cache still count.
 */
(function () {
  'use strict';

  Backdrop.behaviors.aiRelatedContentTracker = {
    attach: function (context, settings) {
      var tracker = settings.aiRelatedContentTracker;
      if (context !== document || !tracker || !tracker.nid || !tracker.url) {
        return;
      }
      if (document.visibilityState === 'prerender') {
        return;
      }

      var key = 'aiRelatedContentHit:' + tracker.nid;
      try {
        if (window.sessionStorage.getItem(key)) {
          return;
        }
        window.sessionStorage.setItem(key, '1');
      }
      catch (e) {
        // Storage blocked: count the view anyway; the server flood limit caps it.
      }

      var body = 'nid=' + encodeURIComponent(tracker.nid);
      if (navigator.sendBeacon) {
        navigator.sendBeacon(tracker.url, new Blob([body], {type: 'application/x-www-form-urlencoded'}));
      }
      else {
        var xhr = new XMLHttpRequest();
        xhr.open('POST', tracker.url, true);
        xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
        xhr.send(body);
      }
    }
  };
})();
