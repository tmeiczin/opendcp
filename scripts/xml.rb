#!/usr/bin/ruby

require 'rubygems'
require 'xmlsimple'

if ARGV.size < 1
  puts "xml.rb dump <ts file>"
  puts "xml.rb xml  <ts file> <translated file>"
  exit
end

if ARGV[0] == "dump"
  in_xml = ARGV[1]

  xml = XmlSimple.xml_in(in_xml)

  xml['context'].each do |i|
    i['message'].each do |j|
        puts j['source']
    end
  end
else
  in_xml = ARGV[1]
  in_txt = ARGV[2]

  t = []
  f = File.open(in_txt) 
  f.each_line do |line|
    t.push line.chomp!
  end

  x = 0

  xml = XmlSimple.xml_in(in_xml)

  xml['context'].each do |i|
     i['message'].each do |j|
       j['translation'][0] = t[x]
       x = x + 1
    end
  end

  root = "TS"
  dec = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"

  puts XmlSimple.xml_out(xml, { 'RootName' => 'TS', 
                                'XmlDeclaration' => dec,
                                'Indent' => '    '
                              }) 
end
