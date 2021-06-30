{% assign local_showcase = include.showcase %}
### {{ local_showcase.name }}
{: .no_toc}

Author: {{ local_showcase.author }}  
{% if local_showcase.ported_by %}Ported by: {{ local_showcase.ported_by }}<br/>{% endif -%}
{% if local_showcase.script_path %}Path: `{{ local_showcase.script_path }}`<br/>{% endif -%}
{% if local_showcase.original_script %}Original script: {{ local_showcase.original_script }}<br/>{% endif -%}
{% if local_showcase.description %}<br/>{{ local_showcase.description  }}<br/>{% endif -%}