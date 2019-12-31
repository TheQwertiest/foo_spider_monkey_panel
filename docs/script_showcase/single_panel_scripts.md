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

{% assign sorted_authors = site.showcase_sample | group_by: "author" | sort_natural : "name" %}
{% for author in sorted_authors %}
{% assign sorted_scripts = author.items | sort_natural : "name" %}
{% for showcase in sorted_scripts %}
### {{ showcase.name }}

Author: {{ author.name }}  
{% if showcase.ported_by %}Ported by: {{ showcase.ported_by }}<br/>{% endif -%}
{% if showcase.script_path %}Path: {{ showcase.script_path }}<br/>{% endif -%}
{% if showcase.original_script %}Original script: {{ showcase.original_script }}<br/>{% endif -%}
{% endfor %}
{% endfor %}

## User scripts

These scripts are created by SMP users and are not installed with the component.  
For installation instructions (and support) see correpsonding links.

{% assign sorted_authors = site.showcase_user | group_by: "author" | sort_natural : "name" %}
{% for author in sorted_authors %}
{% assign sorted_scripts = author.items | sort_natural : "name" %}
{% for showcase in sorted_scripts %}
### {{ showcase.name }}

Author: {{ author.name }}  
{% if showcase.link %}Link: {{ showcase.link }}<br/>{% endif -%}
{% if showcase.support_thread %}Support thread: {{ showcase.support_thread }}<br/>{% endif -%}
{% endfor %}
{% endfor %}
