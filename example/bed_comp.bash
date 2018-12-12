function _bed()
{
  local cmd cur prev sub_cmd
  cmd="$1"
  cur="$2"
  prev="$3"

  if [ $COMP_CWORD -eq 1 ] ; then
    COMPREPLY=($(compgen -W "compute-aci edit mk-bash-comp mk-zsh-comp search validate write-aci write-ber write-def write-id write-indef write-xml " -- "$cur"))
  else
    sub_cmd="${COMP_WORDS[1]}"

    if [[  "$cur" == -* ]]; then
      local opts=""
      case "$sub_cmd" in
        compute-aci)
          opts="--asn --asn-cfg --asn-path --help --indent --no-detect --verbose --version -a -h -v "
          ;;
        edit)
          opts="--asn --asn-cfg --asn-path --command --first --help --no-detect --skip --verbose --version -a -c -h -v "
          ;;
        mk-bash-comp)
          opts="--asn --asn-cfg --asn-path --help --no-detect --output --verbose --version -a -h -o -v "
          ;;
        mk-zsh-comp)
          opts="--asn --asn-cfg --asn-path --help --no-detect --output --verbose --version -a -h -o -v "
          ;;
        search)
          opts="--asn --asn-cfg --asn-path --count --first --help --hex --no-detect --skip --verbose --version -a -e -h -v -x "
          ;;
        validate)
          opts="--asn --asn-cfg --asn-path --first --help --no-detect --skip --verbose --version --xsd -a -h -v "
          ;;
        write-aci)
          opts="--asn --asn-cfg --asn-path --help --no-detect --verbose --version -a -h -v "
          ;;
        write-ber)
          opts="--asn --asn-cfg --asn-path --help --mmap --no-detect --no-fsync --verbose --version -a -h -v "
          ;;
        write-def)
          opts="--asn --asn-cfg --asn-path --help --mmap --no-detect --no-fsync --verbose --version -a -h -v "
          ;;
        write-id)
          opts="--asn --asn-cfg --asn-path --help --mmap --mmap-out --no-detect --no-fsync --verbose --version -a -h -v "
          ;;
        write-indef)
          opts="--asn --asn-cfg --asn-path --help --mmap --no-detect --no-fsync --verbose --version -a -h -v "
          ;;
        write-xml)
          opts="--aci --asn --asn-cfg --asn-path --bci --block --cdr --class --count --first --help --hex --indent --len --length --mmap --no-detect --no-fsync --off --offset --pp --pp-file --search --skip --skip0 --t_size --tag --tl --verbose --version -0 -a -h -v -x "
          ;;
        *)
          ;;
      esac
      COMPREPLY=($(compgen -W "$opts" -- "$cur"))
    else
      if [[ "$sub_cmd" == "edit" && "$prev" == "-c" ]] ; then
        COMPREPLY=($(compgen -W "add insert remove replace set-att set_att write-aci " -- "$cur"))
      else
        COMPREPLY=($(compgen -f -- "$cur"))
      fi
    fi
  fi
}
complete -F _bed bed
