<div id="showcase-grid" data-columns>
{% assign sorted_authors = include.collection | group_by: "author" | sort_natural : "name" %}
{% for author in sorted_authors %}
{% assign sorted_scripts = author.items | sort_natural : "name" %}
{% for showcase in sorted_scripts %}
<div class="showcase">
{% if showcase.img %}
<div class="scriptimgwrap" markdown="0">
  {% capture screenshot_path %}{{ include.screenshots_dir | relative_url }}/{%if showcase.img == "" %}{{ showcase.name }}.png{% else %}{{ showcase.img }}{% endif %} {% endcapture %}
  <a href="{{ screenshot_path }}">
    <img class="scriptimg" alt="Screenshot of a `{{ showcase.name }}` script" text='{{ showcase.name }}' src="{{ screenshot_path }}" />
  </a>
</div>
{% endif %}
<div class="scriptdesc">
{% include functions/{{ include.description_func }}.md showcase=showcase %}
</div>
</div>
{% endfor %}
{% endfor %}
</div>