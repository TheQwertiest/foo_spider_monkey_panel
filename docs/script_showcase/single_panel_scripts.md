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

## Built-in scripts

These scripts are automatically installed with the component.  
To use them replace panel contents with the following line: `include('path_to_script');`  
For example: `include('samples/js-smooth/JS Smooth Browser.js');`

{% include functions/showcase_grid.md collection=site.showcase_sample screenshots_dir='/assets/img/screenshots/samples' description_func='showcase_sample_desc' %}

## User scripts

These scripts are created by SMP users and are not installed with the component.  
For installation instructions (and support) see corresponding links.

{% include functions/showcase_grid.md collection=site.showcase_user screenshots_dir='/assets/img/screenshots/user_scripts' description_func='showcase_user_script_desc' %}

{% include functions/showcase_grid_script.md %}
