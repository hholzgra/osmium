#ifndef OSMIUM_OUTPUT_OSM_XML_HPP
#define OSMIUM_OUTPUT_OSM_XML_HPP

/*

Copyright 2011 Jochen Topf <jochen@topf.org> and others (see README).

This file is part of Osmium (https://github.com/joto/osmium).

Osmium is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License or (at your option) the GNU
General Public License as published by the Free Software Foundation, either
version 3 of the Licenses, or (at your option) any later version.

Osmium is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU Lesser General Public License and the GNU
General Public License for more details.

You should have received a copy of the Licenses along with Osmium. If not, see
<http://www.gnu.org/licenses/>.

*/

// this is required to allow using libxml's xmlwriter in parallel to expat xml parser under debian
#undef XMLCALL
#include <libxml/xmlwriter.h>

// XXX error handling is mostly missing...

namespace Osmium {

    namespace Output {

        namespace OSM {

            class XML : public Base {

                xmlTextWriterPtr xml_writer;

                void write_meta(Osmium::OSM::Object *object) {
                    xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "id",        "%d", object->get_id());
                    xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "version",   "%d", object->get_version());
                    xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "changeset", "%d", object->get_changeset());
                    xmlTextWriterWriteAttribute(xml_writer, BAD_CAST "timestamp", BAD_CAST object->get_timestamp_as_string().c_str());

                    // uid == 0 -> anonymous
                    if (object->get_uid() > 0) {
                        xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "uid", "%d", object->get_uid());
                        xmlTextWriterWriteAttribute(xml_writer, BAD_CAST "user", BAD_CAST object->get_user());
                    }

                    if (m_file.get_type() == OSMFile::FileType::History()) {
                        xmlTextWriterWriteAttribute(xml_writer, BAD_CAST "visible", object->get_visible() ? BAD_CAST "true" : BAD_CAST "false");
                    }
                }

                void write_tags(Osmium::OSM::Object *object) {
                    for (int i=0, l = object->tag_count(); i < l; i++) {
                        xmlTextWriterStartElement(xml_writer, BAD_CAST "tag"); // <tag>
                        xmlTextWriterWriteAttribute(xml_writer, BAD_CAST "k", BAD_CAST object->get_tag_key(i));
                        xmlTextWriterWriteAttribute(xml_writer, BAD_CAST "v", BAD_CAST object->get_tag_value(i));
                        xmlTextWriterEndElement(xml_writer); // </tag>
                    }
                }

            public:

                XML(OSMFile& file) : Base(file) {
                    xml_writer = xmlNewTextWriter(xmlOutputBufferCreateFd(this->get_fd(), NULL));
                }

                ~XML() {
                }

                void write_init() {
                    xmlTextWriterSetIndent(xml_writer, 1);
                    xmlTextWriterSetIndentString(xml_writer, BAD_CAST "  ");
                    xmlTextWriterStartDocument(xml_writer, NULL, "utf-8", NULL); // <?xml .. ?>

                    xmlTextWriterStartElement(xml_writer, BAD_CAST "osm");  // <osm>
                    xmlTextWriterWriteAttribute(xml_writer, BAD_CAST "version", BAD_CAST "0.6");
                    xmlTextWriterWriteAttribute(xml_writer, BAD_CAST "generator", BAD_CAST "Osmium (http://wiki.openstreetmap.org/wiki/Osmium)");
                }

                void write_bounds(double minlon, double minlat, double maxlon, double maxlat) {
                    xmlTextWriterStartElement(xml_writer, BAD_CAST "bounds"); // <bounds>

                    xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "minlon", "%.7f", minlon);
                    xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "minlat", "%.7f", minlat);
                    xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "maxlon", "%.7f", maxlon);
                    xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "maxlat", "%.7f", maxlat);

                    xmlTextWriterEndElement(xml_writer); // </bounds>
                }

                void write(Osmium::OSM::Node* node) {
                    xmlTextWriterStartElement(xml_writer, BAD_CAST "node"); // <node>

                    write_meta(node);

                    xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "lon", "%.7f", node->get_lon());
                    xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "lat", "%.7f", node->get_lat());

                    write_tags(node);

                    xmlTextWriterEndElement(xml_writer); // </node>
                }

                void write(Osmium::OSM::Way* way) {
                    xmlTextWriterStartElement(xml_writer, BAD_CAST "way"); // <way>

                    write_meta(way);

                    for (int i=0, l=way->node_count(); i < l; i++) {
                        xmlTextWriterStartElement(xml_writer, BAD_CAST "nd"); // <nd>
                        xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "ref", "%d", way->get_node_id(i));
                        xmlTextWriterEndElement(xml_writer); // </nd>
                    }

                    write_tags(way);

                    xmlTextWriterEndElement(xml_writer); // </way>
                }

                void write(Osmium::OSM::Relation* relation) {
                    xmlTextWriterStartElement(xml_writer, BAD_CAST "relation"); // <relation>

                    write_meta(relation);

                    for (int i=0, l=relation->member_count(); i < l; i++) {
                        const Osmium::OSM::RelationMember *mem = relation->get_member(i);

                        xmlTextWriterStartElement(xml_writer, BAD_CAST "member"); // <member>

                        switch (mem->get_type()) {
                            case 'n':
                                xmlTextWriterWriteAttribute(xml_writer, BAD_CAST "type", BAD_CAST "node");
                                break;
                            case 'w':
                                xmlTextWriterWriteAttribute(xml_writer, BAD_CAST "type", BAD_CAST "way");
                                break;
                            case 'r':
                                xmlTextWriterWriteAttribute(xml_writer, BAD_CAST "type", BAD_CAST "relation");
                                break;
                        }
                        xmlTextWriterWriteFormatAttribute(xml_writer, BAD_CAST "ref", "%d", mem->get_ref());
                        xmlTextWriterWriteAttribute(xml_writer, BAD_CAST "role", BAD_CAST mem->get_role());

                        xmlTextWriterEndElement(xml_writer); // </member>
                    }

                    write_tags(relation);

                    xmlTextWriterEndElement(xml_writer); // </relation>
                }

                void write_final() {
                    xmlTextWriterEndElement(xml_writer); // </osm>
                    xmlFreeTextWriter(xml_writer);
                    m_file.close();
                }

            }; // class XML

        } // namespace OSM

    } // namespace Output

} // namespace Osmium

#endif // OSMIUM_OUTPUT_OSM_XML_HPP
