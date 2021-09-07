---
title: Playback stats
parent: Frequesntly Asked Questions
nav_order: 3
---

# Playback stats
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

* TOC
{:toc}

---

> This is a topic for intermediate/advanced users with a good understanding of how `Spider Monkey Panel` works with relation to `handles` and `handle lists`. The code snippets assume you're familiar with `callbacks` and how to implement your own buttons, menus, etc. For casual users, there are 2 complete scripts that make use of this database without requiring any script knowledge. See [`Track Rating`]({{ site.baseurl }}{% link docs/script_showcase/single_panel_scripts.md %}#track-rating) and [`Last.fm Loved Tracks Manager`]({{ site.baseurl }}{% link docs/script_showcase/single_panel_scripts.md %}#lastfm-loved-tracks-manager) samples.

## Overview

SMP contains an alternative way to store your playback statistics. The main difference from `foo_playcount` is that these can be set and modified directly.

Note: playback stats are bound to `$lower(%artist% - %title%)` query value.

There are 5 fields available that could be used in foobar2000 queries (e.g. title formatting):

```
%smp_playcount%
%smp_loved%
%smp_first_played%
%smp_last_played%
%smp_rating%
```

The corresponding JavaScript methods are:

```javascript
let handle = fb.GetFocusItem();
handle.SetPlayCount(12); // Must be a whole number, use 0 to clear.
handle.SetLoved(1); // Must be a whole number, use 0 to clear.
handle.SetFirstPlayed("2018-01-12 00:00:00"); // String, use "" to clear.
handle.SetLastPlayed("2018-01-12 00:00:00"); // String, use "" to clear.
handle.SetRating(5); // Must be a whole number, use 0 to clear. Max value can be 10, 20, 100 etc...
handle.ClearStats() // Should be obvious what this does =)
```

`SetFirstPlayed()` and `SetLastPlayed()` can recieve an arbitrary string as an argument, but it's advised to use the same format as in example above, since it matches the format that `foobar2000` uses, which will allow for them to be used in time based [queries](http://wiki.hydrogenaud.io/index.php?title=Foobar2000:Query_syntax#Time_expressions).

After updating value(s) for a `handle`, `RefreshStats()` must be used so that the `foobar2000` core and all other components are made aware of the changes:

```javascript
let handle = fb.GetFocusItem();
handle.SetRating(5);
handle.RefreshStats();
```

When dealing with any kind of bulk operation (i.e. updating many `handles` at once), it's advived to use the `handle list` alternative of `RefreshStats()` for performance reasons:

```javascript
let items = plman.GetPlaylistItems(plman.ActivePlaylist);
for (let i = 0; i < items.Count; i++) {
    items[i].SetRating(5);
}
items.RefreshStats();
```

`RefreshStats()` triggers the `on_metadb_changed()` callback within all Spider Monkey Panel instances and also `on_playback_edited()` if the playing item was updated.

## Storage

All data is stored inside `index-data\E58A4298-065E-469B-B5D6-199106E283DD` which will be inside your `foobar2000` profile folder. This file is exclusive to SMP component. Any other files in this folder will belong to other components (e.g. `foo_playcount`).

Like `foo_playcount`, data is remembered for up to 4 weeks if there is no matching track in the `Media Library` or any playlist.

## Example

Here's a basic example that updates the stats using the same rules as Last.FM (i.e. requirement of minimum playback time):

```javascript
let time_elapsed = 0;
let target_time = 0;

let tf_fp = fb.TitleFormat("%smp_first_played%");
let tf_pc = fb.TitleFormat("$if2(%smp_playcount%,0)");

function get_date() {
    // returns current time formatted like "2018-01-14 09:04:50"
    let d = new Date();
    let t = d.getTime();
    let offset = d.getTimezoneOffset() * 60 * 1000;
    let tmp = new Date(t - offset).toISOString();
    return tmp.substring(0, 10) + ' ' + tmp.substring(11, 19);
}

function on_playback_new_track() {
    time_elapsed = 0; //reset
    // this example uses the same rule as last.fm
    // half the track length or 4 minutes - whichever is lower
    target_time = Math.min(Math.ceil(fb.PlaybackLength / 2), 240);
}

function on_playback_time() {
    time_elapsed++;
    if (time_elapsed == target_time) {
        let date_time = get_date();
        let m = fb.GetNowPlaying();
        let cp = parseInt(tf_pc.Eval()); // get current playcount
        m.SetPlaycount(cp + 1) // increment playcount by 1
        if (tf_fp.Eval() == "") { // only write first played if it's currently empty
            m.SetFirstPlayed(date_time);
        }
        m.SetLastPlayed(date_time) // always write last played
        m.RefreshStats(); // notify foobar2000 core / other components
    }
}
```

## Properties

If you open the `Properties` dialog for a single item, you'll see the full information like this on the `Details` tab.

<details markdown="0">
<summary>
Screenshot
</summary>

{% assign img = "assets/img/misc/stats_single.png" | relative_url %}
{% include functions/clickable_img.html
  img = img
  alt = "Properties of a single item"
  title = "Properties of a single item"
%}
</details>


If you select multiple items, you'll see a calculated total playcount.

<details markdown="0">
<summary>
Screenshot
</summary>

{% assign img = "assets/img/misc/stats_total.png" | relative_url %}
{% include functions/clickable_img.html
  img = img
  alt = "Properties of multiple items"
  title = "Properties of multiple items"
%}
</details>
