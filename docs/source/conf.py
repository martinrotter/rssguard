from datetime import date

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'RSS Guard'
copyright = f'{date.today().year}, Martin Rotter'
author = 'Martin Rotter'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ['sphinx.ext.extlinks']
templates_path = ['_templates']
exclude_patterns = [ 'old_docs' ]

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
html_logo = '../../resources/graphics/rssguard.png'
html_favicon = '../../resources/graphics/rssguard.ico'