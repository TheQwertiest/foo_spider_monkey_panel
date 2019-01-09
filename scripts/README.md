### Main scripts
- setup.bat - Set up everything, so that project can be built.  
  Launch with `--skip_mozjs` to skip SpiderMonkey binaries and headers setup.
- pack_component.bat - Pack project binaries to .fb2k-component archive.

### Utility scripts
- fetch_submodules.bat - Download submodule sources.
- update_submodules.bat - Update submodules to their latest HEADs.
- prepare_scintilla.bat - Configure scintilla.
- patch_submodules.bat - Patch submodules.
- unpack_mozjs.bat - Unpack SpiderMonkey binaries and headers to the proper place.
- generate_commit_hash_header - Generate `commit_hash.h` header, which contains commit hash.

- install_jsdoc.bat - Install jsdoc npm package and required theme.
- create_doc.bat - Create html file from jsodc'd .js file.
