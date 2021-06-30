<div id="showcase-grid" data-columns>
{% if include.sort_by_authors %}
{% assign sorted_authors = include.collection | sort_natural : "author" | group_by: "author" %}
{% else %}
{% assign sorted_authors = include.collection | group_by: "dont_group" %}
{% endif %}
{% for author in sorted_authors %}
{% assign sorted_scripts = author.items | sort_natural : "name" %}
{% for showcase in sorted_scripts %}
<div class="showcase">
{% if showcase.img %}
<div class="scriptimgwrap" id="{% include functions/custom_slugify.md name = showcase.name %}" markdown="0">
  {%- assign screenshots_dir = include.screenshots_dir | relative_url -%}
  {%- capture screenshot_path -%}
    {{ screenshots_dir }}/{%if showcase.img == "" %}{{ showcase.name }}.png{% else %}{{ showcase.img }}{% endif %}
  {%- endcapture -%}
  <a href="{{ screenshot_path }}">
    <img class="scriptimg" alt="Screenshot of a `{{ showcase.name }}` script" title='{{ showcase.name }}' src="{{ screenshot_path }}" />
  </a>
</div>
{% elsif showcase.gallery %}
<div class="scriptimgwrap" id="{% include functions/custom_slugify.md name = showcase.name %}" markdown="0">
  {%- assign screenshots_dir = include.screenshots_dir | relative_url -%}
  {%- capture gallery_name -%}
    {% if showcase.gallery == "" %}{{ showcase.name }}{% else %}{{ showcase.gallery }}{% endif %}
  {%- endcapture -%}
  {%- assign gallery_path = screenshots_dir | append: '/' | append: gallery_name }} -%}
  {% assign gallery_data = site.data.screenshots[gallery_name] %}
  {% assign img_alt = "Screenshot of a `" | append: showcase.name | "` script" %}
  {% include functions/slideshow.html
    gallery_path=gallery_path
    collection=gallery_data
    alt=img_alt
    text=showcase.name
  %}
</div>
{% endif %}
<div class="scriptdesc">
{% include functions/{{ include.description_func }}.md
  showcase=showcase
%}
</div>
</div>
{% endfor %}
{% endfor %}
</div>