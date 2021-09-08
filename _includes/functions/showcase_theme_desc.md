{% assign local_showcase = include.showcase %}
### {{ local_showcase.name }}

Author: {{ local_showcase.author }}  
{% if local_showcase.link %}Link: {{ local_showcase.link }}<br/>{% endif -%}
{% if local_showcase.mirror %}Mirror: {{ local_showcase.mirror }}<br/>{% endif -%}
{% if local_showcase.support_thread %}Support thread: {{ local_showcase.support_thread }}<br/>{% endif -%}
{% if local_showcase.description %}<br/>{{ local_showcase.description  }}<br/>{% endif -%}