# Drush Developers' Toolbox

This is a suite of drush commands that I have crafted to make the Drupal
developer experience a little bit nicer.  Since all of my development is 
currently done on a Mac, these commands have only been tested OS X.

## drush server

Say good-bye to virtual host setups!  This command starts a minimal apache
server from the command-line with your Drupal root as the document root.

## drush console

Run an interactive shell in the current Drupal environment. Requires a valid 
[phpsh](http://phpsh.org/) installation.  Currently only supports Drupal 7, 
though support for Drupal 6 would likely trivial.  *Patches welcome.*

## drush ctags

A simple shortcut for creating a tags file for use in various programs that can
make use of this file for autocompletion and documentation, including phpsh.
*Requires exuberant-ctags.*
