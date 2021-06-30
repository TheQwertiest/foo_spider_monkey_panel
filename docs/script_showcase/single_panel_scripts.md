---
title: Single panel scripts
parent: Script showcase
nav_order: 1
---

# Single panel scripts
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

* TOC
{:toc}

---

## Quick navigate

{% assign collections = "" | split: ',' %}
{% assign collection_types = "" | split: ',' %}
{% assign collections = collections | push: site.showcase_sample %}
{% assign collection_types = collection_types | push: "Built-in" %}
{% assign collections = collections | push: site.showcase_user %}
{% assign collection_types = collection_types | push: "User" %}
{% include functions/showcase_table.html collections=collections collection_types=collection_types %}

## Built-in scripts

These scripts are automatically installed with the component.  
To use them:
- Open `Configure Panel...` dialog.
- In `Script source` choose `Sample`.
- Select the desired script in the drop-down menu.

{% include functions/showcase_grid.md collection=site.showcase_sample screenshots_dir='/assets/img/screenshots/samples' description_func='showcase_sample_desc' sort_by_authors=true %}

## User scripts

These scripts are created by SMP users and are not installed with the component.  
For installation instructions (and support) see corresponding links.

{% include functions/showcase_grid.md collection=site.showcase_user screenshots_dir='/assets/img/screenshots/user_scripts' description_func='showcase_user_script_desc' sort_by_authors=true %}

{% include functions/showcase_grid_script.md %}
