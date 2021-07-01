---
title: JScript migration guide
parent: Guides
nav_order: 2
---

# JScript migration guide
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

* TOC
{:toc}

---

It is rather simple to migrate your script from `foo_jscript_panel` to `foo_spider_monkey_panel`: you only need to perform all the steps listed below.  
Most of these can be automated by using `Find & Replace` command in your favourite text editor.

#### Replace old headers:
<details><summary markdown='span'>Before</summary>

```javascript
// ==PREPROCESSOR==
// @name "MyScript"
// @author "Me"
// @version "1.2.3"
// @import "%fb2k_path%\path\to\script1.js"
// @import "%fb2k_path%\path\to\script2.js"
// @feature "dragdrop"
// ==/PREPROCESSOR==
```
</details>
<details><summary markdown='span'>After</summary>

```javascript
window.DefineScript('MyScript', {author: 'Me', version: '1.2.3', features: {drag_n_drop: true} });
include(`${fb.FoobarPath}/path/to/script1.js`);
include(`${fb.FoobarPath}/path/to/script2.js`);
```
</details><br>

#### Remove `.toArray()` and `.Dispose()` methods:
<details><summary markdown='span'>Before</summary>

```javascript
var artists = tfo.EvalWithMetadbs(handle_list).toArray();
var artist = artists[0];
tfo.Dispose();
```
</details>
<details><summary markdown='span'>After</summary>

```javascript
let artists = tfo.EvalWithMetadbs(handle_list);
let artist = artists[0];
```
</details><br>

#### Replace `FbMetadbHandleList.Item(i)` calls with `FbMetadbHandleList[i]`:
<details><summary markdown='span'>Before</summary>

```javascript
var items = plman.GetPlaylistItems(plman.ActivePlaylist);
var item = items.Item(0);
```
</details>
<details><summary markdown='span'>After</summary>

```javascript
let items = plman.GetPlaylistItems(plman.ActivePlaylist);
let item = items[0];
```
</details><br>

#### Replace `plman.PlaylistRecyclerManager` with `plman.PlaylistRecycler`; replace calls to it's properties: `Name`>`GetName`, `Content`>`GetContent`:
<details><summary markdown='span'>Before</summary>

```javascript
var playlist_name = plman.PlaylistRecyclerManager.Name(i);
var playlist_content = plman.PlaylistRecyclerManager.Content(i);
```
</details>
<details><summary markdown='span'>After</summary>

```javascript
let playlist_name = plman.PlaylistRecycler.GetName(i);
let playlist_content = plman.PlaylistRecycler.GetContent(i);
```
</details><br>

#### Add additional argument to `fb.DoDragDrop`:
<details><summary markdown='span'>Before</summary>

```javascript
fb.DoDragDrop(cur_playlist_selection, g_drop_effect.copy);
```
</details>
<details><summary markdown='span'>After</summary>

```javascript
fb.DoDragDrop(0, cur_playlist_selection, g_drop_effect.copy);
```
</details><br>

#### If there are errors about unknown methods or properties, make sure that corresponding methods and properties have the valid case:
<details><summary markdown='span'>Before</summary>

```javascript
Console.Log('Log message');
var items = plman.getPlaylistItems(plman.activePlaylist);
```
</details>
<details><summary markdown='span'>After</summary>

```javascript
console.log('Log message');
let items = plman.GetPlaylistItems(plman.ActivePlaylist);
```
</details><br>

#### Some methods have more rigorous error checks, so you'll have to pass the valid arguments:
<details><summary markdown='span'>Before</summary>

```javascript
menu.CheckMenuRadioItem(StartIndex, StartIndex, StartIndex + (checked ? 0 : 1)); ///< out of bounds error
```
</details>
<details><summary markdown='span'>After</summary>

```javascript
if (checked) {
    menu.CheckMenuRadioItem(StartIndex, StartIndex, StartIndex);
}
```
</details>
