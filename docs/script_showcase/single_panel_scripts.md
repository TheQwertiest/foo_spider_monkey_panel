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

{% for showcase in site.showcase_sample %}
### {{ showcase.name }}

Author: {{ showcase.author }}  
{% if showcase.ported_by %}Ported by: {{ showcase.ported_by }}<br/>{% endif -%}
{% if showcase.script_path %}Path: {{ showcase.script_path }}<br/>{% endif -%}
{% if showcase.original_script %}Original script: {{ showcase.original_script }}<br/>{% endif -%}
{% endfor %}

## User scripts

These scripts are created by SMP users and are not installed with the component.  
For installation instructions (and support) see correpsonding links.

{% for showcase in site.showcase_user %}
### {{ showcase.name }}

Author: {{ showcase.author }}  
{% if showcase.link %}Link: {{ showcase.link }}<br/>{% endif -%}
{% if showcase.support_thread %}Support thread: {{ showcase.support_thread }}<br/>{% endif -%}
{% endfor %}
