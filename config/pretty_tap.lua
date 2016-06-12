
require('tel-codes')


-- ber_class = 64



function pp_optional(x)
  if x == nil then return '' else return ' ' .. x end
end

function scale_str(s, i)
  local n = string.len(s)
  if i >= n then
    return '0.' .. string.rep('0', i-n) .. s
  else
    return string.sub(s, 1, n-i) .. '.' .. string.sub(s, n-i+1, -1)
  end
end

function scale_int(i, j)
  return i/10^j
end

function scale_int_s(i, j)
  return scale_str(tostring(i), j)
end

decimal_places = 6

function store_decimal_places(i)
  decimal_places = i
end

function pp_charge(i)
  local charge = scale_int_s(i, decimal_places)
  return string.format('%s %s', charge, tap_currency)
end

utc_map = {}
utc_code = nil

function pp_off(s)
  return string.gsub(s, '^(.)(%d%d)(%d%d)$', '%1%2:%3')
end

function utc_push(tag, value)
  if tag == 232 then
    utc_code = value
  else
    utc_map[utc_code] = pp_off(value)
  end
end

function pp_utc(code)
  return utc_map[code]
end

function pp_date_time(s)
  r, _ = string.gsub(s, '^(%d%d%d%d)(%d%d)(%d%d)(%d%d)(%d%d)(%d%d)$',
      '%1-%2-%3 %4:%5:%6')
  return r
end

function pp_charging_point(c)
  if c == 'C' then
    return 'Completion'
  elseif c == 'D' then
    return 'Deposit'
  else
    return 'nil'
  end
end

ccp_map = {
  [1] = 'Order Placed',
  [2] = 'Requested Delivery',
  [3] = 'Actual Delivery'
}

function pp_content_charging_point(i)
  return ccp_map[i]
end

tax_type_map = {
  ['01'] = 'national',
  ['02'] = 'regional',
  ['03'] = 'county',
  ['04'] = 'local/city'
}

function pp_tax_type(tt)
  return tax_type_map[tt]
end

ct_map = {
  ['00'] = 'total',
  ['01'] = 'airtime',
  ['02'] = 'reserved',
  ['03'] = 'toll',
  ['04'] = 'directory assistance',
  ['21'] = 'VPMN surcharge',
  ['50'] = 'total IOT'
}

function pp_charge_type(t)
  -- needs base library
  -- if tonumber(t) >= 69 and tonumber(t) <= 99 then
  if string.len(t) == 2 and t >= '69' and t <= '99' then
    return 'reserved'
  else
    return ct_map[t]
  end
end

service_map = {
  ['00'] = 'all teleservices',
  ['10'] = 'all speech', -- .. 'transmission services',
  ['11'] = 'telephony',
  ['12'] = 'emergency calls',
  ['20'] = 'all SMS services',
  ['21'] = 'short message MT/PP',
  ['22'] = 'short message MO/PP',
  ['60'] = 'all FAX services',
  ['61'] = 'facsimile group 3 & alternative speech',
  ['62'] = 'automatic facsimile group 3',
  ['63'] = 'automatic facsimile group 4',
  ['70'] = 'all data teleservices (compound)',
  ['80'] = 'all teleservices except SMS (compound)',
  ['90'] = 'all voice group call services',
  ['91'] = 'voice group call',
  ['92'] = 'voice broadcast call',
}

function pp_tele_service(s)
  return service_map[s]
end

ci_map = {
  ['A'] = 'call set up attempt',
  ['C'] = 'content',
  ['D'] = 'duration',
  ['E'] = 'event',
  ['F'] = 'fixed',
  ['L'] = 'calendar based usage',
  ['V'] = 'outgoing volume',
  ['W'] = 'incoming volume',
  ['X'] = 'total volume',
}

last_ci = nil

function pp_ci(s)
  last_ci = s
  return ci_map[s]
end

mcc_region_map = {
  ['0'] = 'Test network',
  ['2'] = 'Europe',
  ['3'] = 'North America and the Caribbean',
  ['4'] = 'Asia and the Middle East',
  ['5'] = 'Oceania',
  ['6'] = 'Africa',
  ['7'] = 'South and Central America',
  ['9'] = 'World-wide',
}




function pp_imsi(s)
  local mcc = string.sub(s, 1, 3)
  local region = string.sub(s, 1, 1)
  local p5 = string.sub(s, 1, 5)
  local p6 = string.sub(s, 1, 6)
  local mnc = 'unk'
  local nw = mcc_mnc_map[p6]
  if nw == nil then
    nw = mcc_mnc_map[p5]
    if nw == nil then
      nw = 'unk'
    else
      mnc = string.sub(s, 4, 5)
    end
  else
    mnc = string.sub(s, 4, 6)
  end
  return string.format('MCC="%s (%s)", region="%s (%s)", MNC="%s (%s)"',
      mcc_map[mcc], mcc, mcc_region_map[region], region, nw, mnc)
end

function pp_pmn_code(s)
  local alpha3 = string.sub(s, 1, 3)
  local rest = string.sub(s, 4)
  return string.format('%s (%s), %s"', iso_alpha3_map[alpha3], alpha3, rest)
end


function pp_cur_code(s)
  return cur_map[s]
end

tap_currency = 'SDR'

function pp_tap_cur(s)
  tap_currency = s
  return pp_cur_code(s)
end

local_currency = ''

function pp_local_cur(s)
  local_currency = s
  return pp_cur_code(s)
end




function pp_cc(s)
  local ccn = nil
  local ccs = nil
  for x = 7, 1, -1 do
    ccn = string.sub(s, 1, x)
    ccs = cc_map[ccn]
    -- ccs = get_cc_map(ccn)
    -- ccs = cc_map[tonumber(ccn)]
    if ccs ~= nil then break end
  end
  local cc_p = ''
  if ccs ~= nil then cc_p = string.format('cc="%s (%s)" ', ccs, ccn) end
  return cc_p
end



function pp_prefix_code(s, key, min, max, map)
  for i = max, min, -1 do
    local code = string.sub(s, 1, i)
    local area = map[code]
    if area ~= nil then
      return string.format('%s="%s (%s)" ', key, area, code)
    end
  end
  return ''
end

function pp_area_code(s)
  return pp_prefix_code(s, 'area', 3, 9, area_code_map)
end

function pp_carrier_code(s)
  return pp_prefix_code(s, 'carrier', 3, 9, carrier_code_map)
end

function pp_number(s)
  local cc = pp_cc(s)
  local ac = pp_area_code(s)
  local cac = pp_carrier_code(s)
  return cc .. ac .. cac
end

bearer_code_map = {
  ['00'] = 'All Bearer Services',
  ['20'] = 'All Data Circuit Asynchronous Services',
  ['21'] = 'Duplex Asynchronous 300bps data circuit',
  ['22'] = 'Duplex Asynchronous 1200bps data circuit',
  ['23'] = 'Duplex Asynchronous 1200/75bps data circuit',
  ['24'] = 'Duplex Asynchronous 2400bps data circuit',
  ['25'] = 'Duplex Asynchronous 4800bps data circuit',
  ['26'] = 'Duplex Asynchronous 9600bps data circuit',
  ['27'] = 'General Data Circuit Asynchronous Service',
  ['30'] = 'All Data Circuit Synchronous Services',
  ['32'] = 'Duplex Synchronous 1200bps data circuit',
  ['34'] = 'Duplex Synchronous 2400bps data circuit',
  ['35'] = 'Duplex Synchronous 4800bps data circuit',
  ['36'] = 'Duplex Synchronous 9600bps data circuit',
  ['37'] = 'General Data Circuit Synchronous Service',
  ['40'] = 'All Dedicated PAD Access Services',
  ['41'] = 'Duplex Asynchronous 300bps PAD access',
  ['42'] = 'Duplex Asynchronous 1200bps PAD access',
  ['43'] = 'Duplex Asynchronous 1200/75bps PAD access',
  ['44'] = 'Duplex Asynchronous 2400bps PAD access',
  ['45'] = 'Duplex Asynchronous 4800bps PAD access',
  ['46'] = 'Duplex Asynchronous 9600bps PAD access',
  ['47'] = 'General PAD Access Service',
  ['50'] = 'All Dedicated Packet Access Services',
  ['54'] = 'Duplex Synchronous 2400bps PAD access',
  ['55'] = 'Duplex Synchronous 4800bps PAD access',
  ['56'] = 'Duplex Synchronous 9600bps PAD access',
  ['57'] = 'General Packet Access Service',
  ['60'] = 'All Alternat Speech/Asynchronous Services',
  ['70'] = 'All Alternate Speech/Synchronous Services',
  ['80'] = 'All Speech followed by Data Asynchronous Services',
  ['90'] = 'All Speech followed by Data Synchronous Services',
  ['A0'] = 'All Data Circuit Asynchronous Services (compound)',
  ['B0'] = 'All Data Circuit Synchronous Services (compound)',
  ['C0'] = 'All Asynchronous Services (compound)',
  ['D0'] = 'All Synchronous Services (compound)',
}

function pp_bearer_code(s)
  return bearer_code_map[s]
end

suppl_code_map = {
  ['00'] = 'All supplementary services',
  ['10'] = 'All line identification services',
  ['11'] = 'Calling number identification presentation',
  ['12'] = 'Calling number identification restriction',
  ['13'] = 'Connected number identification presentation',
  ['14'] = 'Connected number identification restriction',
  ['18'] = 'All name identification Supplementary Service',
  ['19'] = 'Calling name presentation',
  ['20'] = 'All call forwarding',
  ['21'] = 'Call forwarding unconditional',
  ['24'] = 'Call deflection',
  ['28'] = 'All conditional Call Forwarding',
  ['29'] = 'Call forwarding on mobile subscriber busy',
  ['2A'] = 'Call forwarding on no reply',
  ['2B'] = 'Call forwarding on subscriber not reachable',
  ['30'] = 'All call offering services',
  ['31'] = 'Call transfer',
  ['40'] = 'All call completion services',
  ['41'] = 'Call waiting',
  ['42'] = 'Call hold',
  ['43'] = 'Completion of calls to busy subscribers (origin)',
  ['44'] = 'Completion of calls to busy subscribers (destination)',
  ['45'] = 'Multicall',
  ['50'] = 'All multi party services',
  ['51'] = 'Multi party service',
  ['60'] = 'All community of interest services',
  ['61'] = 'Closed user groups',
  ['70'] = 'All charging supplement services',
}

function pp_suppl_code(s)
  return suppl_code_map[s]
end

camel_level_map = {
  [0] = 'basic',
  [1] = 'medium',
  [2] = 'high',
  [3] = 'maximum',
}

function pp_camel_level(i)
  return camel_level_map[i]
end

cause_for_term_map = {
  [1]  = 'unsuccessful service delivery',
  [3]  = 'unsuccessful call attempt',
  [4]  = 'stable call abnormal termination',
  [5]  = 'CAMEL initiated call release/management intervention',
  [20] = 'management intervention',
  [21] = 'intra SGSN intersystem change',
  [24] = 'SGSN PLMNIDs change',
}

function pp_cause_for_term(i)
  return cause_for_term_map[i]
end

ctl1_map = {
  [0]   = 'n/a',
  [1]   = 'national',
  [2]   = 'international',
  [10]  = 'HGGSN/HP-GW',
  [11]  = 'VGGSN/VP-GW',
  [12]  = 'other GGSN/P-GW',
  [100] = 'Wi-Fi',
}

function pp_ctl1(i)
  return ctl1_map[i]
end

ctl2_map = {
  [0]  = 'n/a',
  [1]  = 'mobile',
  [2]  = 'PSTN',
  [3]  = 'non-geographic',
  [4]  = 'premium rate',
  [5]  = 'satellite destination',
  [6]  = 'forwarded call',
  [7]  = 'non-forwarded call',
  [10] = 'broadband',
  [11] = 'narrowband',
  [12] = 'conversational',
  [13] = 'streaming',
  [14] = 'interactive',
  [15] = 'background',
}

function pp_ctl2(i)
  return ctl2_map[i]
end

function pp_volume(i)
  if i >= 1024 * 1024 * 1024 then
    return string.format('%.3f GiB', i/(1024 * 1024 * 1024))
  elseif i >= 1024 * 1024 then
    return string.format('%.3f MiB', i/(1024 * 1024))
  elseif i >= 1024 then
    return string.format('%.3f KiB', i/1024)
  else
    return string.format('%d bytes', i)
  end
end

function pp_duration(i)
  local x = i
  -- lua-jit 5.1 doesn't have the // operator ...
  -- local hours = x // 3600
  local hours = math.floor(x / 3600)
  x = x - hours * 3600
  -- local minutes = x // 60
  local minutes = math.floor(x / 60)
  x = x - minutes * 60
  local seconds = x
  local hs = ''
  if hours > 0 then hs = hours .. 'h' end
  local ms = ''
  if minutes > 0 then ms = minutes .. 'm' end
  local ss = ''
  if seconds > 0 then ss = seconds .. 's' end
  return string.format('%s%s%s', hs, ms, ss)
end

function pp_units(i)
  if last_ci == 'X' or last_ci == 'V' or last_ci == 'W' then
    return pp_volume(i)
  else
    return pp_duration(i)
  end
end

-- XXX correct codes?
element_type_map = {
  [10] = 'Short Message Service Centre (SMSC)',
  [20] = 'Serving Call Session Control Function (S-CSCF)',
  [30] = 'Short Message IP Gateway (SM-IP-GW)',
}

function pp_element_type(i)
  return element_type_map[i]
end

function pp_tax_indicator(s)
  if s == '1' then
    return 'Value Added Tax'
  else
    return 'TaxIndicator=' .. s
  end
end

function pp_tax_rate(i)
  return string.gsub(scale_str(i, 5), '^0(%d)', '%1') .. ' %'
end

tax_map = {}
taxation = {}


function tax_push(tag, value)
  if tag == 212 then -- TaxCode
    tax_store()
    taxation = { ['code'] = value }
  elseif tag == 217 then -- TaxType
    taxation['type'] = pp_tax_type(value)
  elseif tag == 215 then -- TaxRate
    taxation['rate'] = pp_tax_rate(value)
  elseif tag == 71 then -- ChargeType
    taxation['ct'] = value .. ' (' .. pp_charge_type(value) .. ')'
  elseif tag == 432 then -- TaxIndicator
    taxation['indicator'] = pp_tax_indicator(value)
  end
end

function tax_store()
  if taxation['code'] == nil then
    return
  end
  tax_map[taxation['code']] = taxation['type']
      .. pp_optional(taxation['rate'])
      .. pp_optional(taxation['ct'])
      .. pp_optional(taxation['indicator'])
end

function pp_tax(code)
  return tax_map[code]
end

exr_map = {}
exr = {}

function exr_push(tag, value)
  if tag == 105 then -- ExchangeRateCode
    exr['code'] = value
  elseif tag == 159 then -- NumberOfDecimalPlaces
    exr['places'] = value
  elseif tag == 104 then -- ExchangeRate
    --exr_map[exr['code']] = scale_int_s(value, exr['places'])
    exr_map[exr['code']] = pp_exr_def(value)
  end
end

function pp_exr(code)
  return exr_map[code]
end

function pp_exr_def(i)
  return string.format('%s %s = 1 %s', scale_int_s(i, exr['places']),
    local_currency, tap_currency)
end

rec_type_map = {
  [1]  = 'Mobile Services Switching Centre (MSC)',
  [2]  = 'Short Message Service Centre (SMSC)',
  [3]  = 'Gateway GPRS Support Node (GGSN) or PDN Gateway (P-GW)',
  [4]  = 'Serving GPRS Support Node (SGSN)',
  [5]  = 'Gateway Mobile Location Centre (GMLC)',
  [6]  = 'Wi-Fi Billing Information Recording Entity',
  [7]  = 'PDN Gateway (P-GW)',
  [8]  = 'Serving Gateway (S-GW)',
  [9]  = 'Proxy Call Session Control Function (P-CSCF)',
  [10] = 'Transit and Roaming Function (TRF)',
  [11] = 'Access Transfer Control Function (ATCF)',
}

function pp_rec_type(i)
  return rec_type_map[i]
end

rec_entity_map = {}
rec_entity = {}

function rec_entity_push(tag, value)
  if tag == 184 then -- RecEntityCode
    rec_entity = { ['code'] = value }
  elseif tag == 186 then -- RecEntityType
    rec_entity['type'] = value
  else
    rec_entity_map[rec_entity['code']] = string.format('%s (%s)',
      value, pp_rec_type(rec_entity['type']))
  end
end

function pp_rec_entity(code)
  return rec_entity_map[code]
end

function pp_nw_context(i)
  if i == 1 then return 'network initiated' end
end

suppl_action_map = {
  [0] = 'registration',
  [1] = 'erasure',
  [2] = 'activation',
  [3] = 'deactivation',
  [4] = 'interrogation',
  [5] = 'invocation',
  [6] = 'registration of password',
  [7] = 'USSD invocation',
}

function pp_suppl_action(i)
  return suppl_action_map[i]
end

partial_map = {
  ['F'] = 'first',
  ['I'] = 'intermediate',
  ['L'] = 'last',
}

function pp_partial(s)
  return partial_map[s]
end

function pp_call_handling(i)
  if i == 0 then return 'continue' elseif i == 1 then return 'release' end
end

function pp_transparency(i)
  if i == 0 then return 'transparent UMTS mode'
  elseif i == 1 then return 'non-transparent UMTS mode' end
end

function pp_clir(i)
  if i == 0 then return 'presentation allowed'
  elseif i == 1 then return 'presentation forbidden' end
end


-- must come after function definitions
-- otherwise table contains nil entries ...

xpath_callback = {
  ['Currency'] = {
    ['path'] = 'TransferBatch/AccountingInfo/CurrencyConversionList/CurrencyConversion',
    ['push'] = exr_push
  },
  ['Taxation'] = {
    ['path'] = 'TransferBatch/AccountingInfo/TaxationList/Taxation',
    ['push'] = tax_push,
    ['store'] = tax_store

  },
  ['UtcTimeOffsetInfo'] = {
    ['path'] = 'TransferBatch/NetworkInfo/UtcTimeOffsetInfoList/UtcTimeOffsetInfo',
    ['push']  = utc_push
  },
  ['RecEntityInformation'] = {
    ['path'] = 'TransferBatch/NetworkInfo/RecEntityInfoList/RecEntityInformation',
    ['push'] = rec_entity_push
  }
}


tag_callback = {
  -- Charge
  [62] = pp_charge,
  -- TotalCharge
  [415] = pp_charge,
  -- Commission
  [350] = pp_charge,
  -- AdvisedCharge
  [349] = pp_charge,
  -- TaxValue
  [397] = pp_charge,
  -- TaxableAmount
  [398] = pp_charge,
  -- TotalTaxValue
  [226] = pp_charge,
  -- TotalDiscountValue
  [225] = pp_charge,
  -- TotalDiscountRefund
  [354] = pp_charge,
  -- CamelInvocationFee
  [422] = pp_charge,
  -- ChargeType
  [71] = pp_charge_type,
  -- ExchangeRateCode
  [105] =  pp_exr,
  -- TaxCode
  [212] = pp_tax,
  -- TaxType
  [217] = pp_tax_type,
  -- TaxRate
  [215] = pp_tax_rate,
  -- UtcTimeOffset
  [231] = pp_off,
  -- UtcTimeOffsetCode
  [232] = pp_utc,
  -- TapDecimalPlaces
  [244] = store_decimal_places,
  -- LocalTimeStamp
  [16] = pp_date_time,
  -- ChargingPoint
  [73] = pp_charging_point,
  -- ContentChargingPoint
  [345] = pp_content_charging_point,
  -- TeleServiceCode
  [218] = pp_tele_service,
  -- ChargedItem
  [66] = pp_ci,
  -- Imsi
  [129] = pp_imsi,
  --  Sender
  [196] = pp_pmn_code,
  -- Recicpient
  [182] = pp_pmn_code,
  -- LocalCurrency
  [135] = pp_local_cur,
  -- TapCurrency
  [210] = pp_tap_cur,
  -- AdvisedChargeCurrency
  [348] = pp_cur_code,
  -- ExchangeRate
  [104] = pp_exr_def,
  -- RecEntityType
  [186] = pp_rec_type,
  -- RecEntityCode
  [184] = pp_rec_entity,
  -- BearerServiceCode
  [40] = pp_bearer_code,
  -- SupplServiceCode
  [209] = pp_suppl_code,
  -- CamelServiceLevel
  [56] = pp_camel_level,
  -- CauseForTerm
  [58] = pp_cause_for_term,
  -- CallTypeLevel1
  [259] = pp_ctl1,
  -- CallTypeLevel2
  [255] = pp_ctl2,
  -- DataVolumeIncoming
  [250] = pp_volume,
  -- DataVolumeOutgoing
  [251] = pp_volume,
  -- TotalDataVolume
  [343] = pp_volume,
  -- TotalCallEventDuration
  [223] = pp_duration,
  -- ElementType
  [438] = pp_element_type,
  -- ChargeableUnits
  [65] = pp_units,
  -- ChargedUnits
  [68] = pp_units,
  -- CalledNumber
  [407] = pp_number,
  -- DialledDigits
  [279] = pp_number,
  -- SMSDestinationNumber
  [419] = pp_number,
  -- NonChargedPartyNumber
  [444] = pp_number,
  -- RequestedNumber
  [451] = pp_number,
  -- Msisdn
  [152] = pp_number,
  -- CamelDestinationNumber
  [404] = pp_number,
  -- CallingNumber
  [405] = pp_number,
  -- ThirdPartyNumber
  [403] = pp_number,
  -- NetworkInitPDPContext
  [245] = pp_nw_context,
  -- SupplServiceActionCode
  [208] = pp_suppl_action,
  -- TaxIndicator
  [432] = pp_tax_indicator,
  -- PartialTypeIndicator
  [166] = pp_partial,
  -- DefaultCallHandlingIndicator
  [87] = pp_call_handling,
  -- TransparencyIndicator
  [228] = pp_transparency,
  -- ClirIndicator
  [75] = pp_clir,
}

