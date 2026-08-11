project = "c-capnproto"
copyright = "2026, Rohit Goswami"
author = "Rohit Goswami"
release = "0.3.0"

extensions = [
    "sphinx_copybutton",
    "sphinx_design",
    "myst_parser",
    "sphinxcontrib.mermaid",
]

templates_path = ["_templates"]
exclude_patterns = ["_build"]

myst_enable_extensions = ["colon_fence", "deflist"]

html_theme = "shibuya"
html_static_path = ["_static"]
html_title = "c-capnproto documentation"
html_baseurl = "https://c-capnproto.rgoswami.me/"

html_context = {
    "source_type": "github",
    "source_user": "HaoZeke",
    "source_repo": "c-capnproto",
    "source_version": "main",
    "source_docs_path": "/docs/orgmode/",
}

html_sidebars = {
    "**": [
        "sidebars/localtoc.html",
        "sidebars/repo-stats.html",
        "sidebars/edit-this-page.html",
    ],
}

html_theme_options = {
    "github_url": "https://github.com/HaoZeke/c-capnproto",
    "accent_color": "blue",
    "dark_code": True,
    "globaltoc_expand_depth": 1,
    "nav_links": [
        {"title": "Install", "url": "install", "summary": "Meson, CMake, autotools"},
        {"title": "Tutorial", "url": "tutorial", "summary": "AddressBook write and read"},
        {"title": "Usage", "url": "usage", "summary": "capnpc-c and the C runtime"},
        {"title": "Wire", "url": "wire", "summary": "Schema-order, packed, canonical"},
        {
            "title": "Ecosystem",
            "children": [
                {
                    "title": "capnp-fortran",
                    "url": "https://capnp-fortran.rgoswami.me",
                    "summary": "F2018 runtime, plugin, RPC",
                    "external": True,
                },
                {
                    "title": "capnp-janet",
                    "url": "https://capnp-janet.rgoswami.me",
                    "summary": "Janet / C wire runtime",
                    "external": True,
                },
            ],
        },
        {"title": "GitHub", "url": "https://github.com/HaoZeke/c-capnproto", "external": True},
    ],
}

copybutton_prompt_text = r">>> |\.\.\. |\$ |In \[\d*\]: | {2,5}\.\.\.: | {5,8}: "
copybutton_prompt_is_regexp = True
copybutton_exclude = ".linenos, .gp, .go"
