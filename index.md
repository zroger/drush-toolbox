---
layout: project
---

## Drush Developers' Toolbox

This is a suite of drush commands that I have crafted to make the Drupal
developer experience a little bit nicer.  Since all of my development is 
currently done on a Mac, these commands have only been tested OS X.

<dl>
  <dt>drush server</dt>
  <dd>
    Say good-bye to virtual host setups!  This command starts a minimal apache
    server from the command-line with your Drupal root as the document root.
  </dd>

  <dt>drush console</dt>
  <dd>
    Run an interactive shell in the current Drupal environment. Requires a valid 
    [phpsh](http://phpsh.org/) installation.
  </dd>

  <dt>drush ctags</dt>
  <dd>
    A simple shortcut for creating a tags file for use in various programs that can
    make use of this file for autocompletion and documentation, including phpsh.
    *Requires exuberant-ctags.*
  </dd>
</dl>
