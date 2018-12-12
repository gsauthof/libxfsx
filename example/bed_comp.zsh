#compdef _bed bed

function _bed {
    local line

    _arguments -C \
        "-h[display help text and exit]" \
        "--help[display help text and exit]" \
"1:command:((compute-aci\:'Compute Audit Control Info' edit\:'Edit BER file in memory' mk-bash-comp\:'Print Bash completion file' mk-zsh-comp\:'Print Zsh completion file' search\:'XPath search in BER file with libxml2' validate\:'Validate BER file with libxml2' write-aci\:'Rewrite Audit Control Info' write-ber\:'Convert XML to BER' write-def\:'Convert BER to all definite tags' write-id\:'Decode/encode BER' write-indef\:'Convert BER to all indefinite tags' write-xml\:'Convert BER to XML' ))" \
      "*::arg:->args"

    case $line[1] in
        compute-aci)
            _bed_compute_aci
            ;;
        edit)
            _bed_edit
            ;;
        mk-bash-comp)
            _bed_mk_bash_comp
            ;;
        mk-zsh-comp)
            _bed_mk_zsh_comp
            ;;
        search)
            _bed_search
            ;;
        validate)
            _bed_validate
            ;;
        write-aci)
            _bed_write_aci
            ;;
        write-ber)
            _bed_write_ber
            ;;
        write-def)
            _bed_write_def
            ;;
        write-id)
            _bed_write_id
            ;;
        write-indef)
            _bed_write_indef
            ;;
        write-xml)
            _bed_write_xml
            ;;
    esac
}

function _bed_compute_aci {
    _arguments \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--indent[Indentation step size]" \
        "--no-detect[disable autodetection]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-v[increase verbosity]" \
        1:input:_files \
        2:output:_files
}

function _bed_edit {
    _arguments \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--command[edit command]" \
        "--first[stop reading after the first element]" \
        "--no-detect[disable autodetection]" \
        "--skip[skip N input bytes]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-c[edit command]" \
        "-v[increase verbosity]" \
        1:input:_files \
        2:output:_files
}

function _bed_mk_bash_comp {
    _arguments \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--no-detect[disable autodetection]" \
        "--output[output file]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-o[output file]" \
        "-v[increase verbosity]" \
        1:input:_files \
        2:output:_files
}

function _bed_mk_zsh_comp {
    _arguments \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--no-detect[disable autodetection]" \
        "--output[output file]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-o[output file]" \
        "-v[increase verbosity]" \
        1:input:_files \
        2:output:_files
}

function _bed_search {
    _arguments \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--count[just print the first N tags]" \
        "--first[stop reading after the first element]" \
        "--hex[include hex-dump of primitive content]" \
        "--no-detect[disable autodetection]" \
        "--skip[skip N input bytes]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-e[XPath expresion]" \
        "-v[increase verbosity]" \
        "-x[include hex-dump of primitive content]" \
        1:input:_files \
        2:output:_files
}

function _bed_validate {
    _arguments \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--first[stop reading after the first element]" \
        "--no-detect[disable autodetection]" \
        "--skip[skip N input bytes]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "--xsd[XSD schema for validation]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-v[increase verbosity]" \
        1:input:_files \
        2:output:_files
}

function _bed_write_aci {
    _arguments \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--no-detect[disable autodetection]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-v[increase verbosity]" \
        1:input:_files \
        2:output:_files
}

function _bed_write_ber {
    _arguments \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--mmap[memory-map input]" \
        "--no-detect[disable autodetection]" \
        "--no-fsync[skip fsync/msync after the last write]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-v[increase verbosity]" \
        1:input:_files \
        2:output:_files
}

function _bed_write_def {
    _arguments \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--mmap[memory-map input]" \
        "--no-detect[disable autodetection]" \
        "--no-fsync[skip fsync/msync after the last write]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-v[increase verbosity]" \
        1:input:_files \
        2:output:_files
}

function _bed_write_id {
    _arguments \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--mmap[memory-map input]" \
        "--mmap-out[memory-map output]" \
        "--no-detect[disable autodetection]" \
        "--no-fsync[skip fsync/msync after the last write]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-v[increase verbosity]" \
        1:input:_files \
        2:output:_files
}

function _bed_write_indef {
    _arguments \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--mmap[memory-map input]" \
        "--no-detect[disable autodetection]" \
        "--no-fsync[skip fsync/msync after the last write]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-v[increase verbosity]" \
        1:input:_files \
        2:output:_files
}

function _bed_write_xml {
    _arguments \
        "--aci[just print the AuditControlInfo]" \
        "--asn[ASN.1 grammar file]:asn:_files" \
        "--asn-cfg[ASN.1 autoconfig .json]" \
        "--asn-path[ASN.1 search path]" \
        "--bci[only read the first header tags]" \
        "--block[read in N byte blocks, skip fillers]" \
        "--cdr[just print the nth or n,m,o-p CDR(s)]" \
        "--class[include class numbers]" \
        "--count[just print the first N tags]" \
        "--first[stop reading after the first element]" \
        "--hex[include hex-dump of primitive content]" \
        "--indent[Indentation step size]" \
        "--len[include content lengths]" \
        "--length[include content lengths]" \
        "--mmap[memory-map input]" \
        "--no-detect[disable autodetection]" \
        "--no-fsync[skip fsync/msync after the last write]" \
        "--off[include byte offsets]" \
        "--offset[include byte offsets]" \
        "--pp[pretty print content]" \
        "--pp-file[pretty print Lua file]" \
        "--search[only print what matches a simple PATH]" \
        "--skip[skip N input bytes]" \
        "--skip0[skip trailing zero bytes]" \
        "--t_size[include tag encoding size]" \
        "--tag[include tag numbers]" \
        "--tl[include tag-length encoding size]" \
        "--verbose[increase verbosity]" \
        "--version[display program version]" \
        "-0[skip trailing zero bytes]" \
        "-a[ASN.1 grammar file]:asn:_files" \
        "-v[increase verbosity]" \
        "-x[include hex-dump of primitive content]" \
        1:input:_files \
        2:output:_files
}

