// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.data.access.simple;

import com.yahoo.data.access.ArrayTraverser;
import com.yahoo.data.access.Inspectable;
import com.yahoo.data.access.Inspector;
import com.yahoo.data.access.ObjectTraverser;

/**
 * Encodes json from an inspectable object.
 *
 * @author arnej27959
 */
public final class JsonRender {

    public static StringBuilder render(Inspectable value,
                                       StringBuilder target,
                                       boolean compact) {
        return render(value, new StringEncoder(target, compact));
    }

    /**
     * Renders the given value to the target stringbuilder with a given encoder.
     * This is useful to use an encoder where rendering of some value types is customized.
     */
    public static StringBuilder render(Inspectable value, StringEncoder encoder) {
        encoder.encode(value.inspect());
        return encoder.target();
    }

    public static class StringEncoder implements ArrayTraverser, ObjectTraverser {

        private final StringBuilder out;
        private boolean head = true;
        private final boolean compact;
        private int level = 0;

        public StringEncoder(StringBuilder out, boolean compact) {
            this.out = out;
            this.compact = compact;
        }

        public void encode(Inspector top) {
            encodeValue(top);
            if (!compact) {
                out.append('\n');
            }
        }

        protected void encodeEMPTY() {
            out.append("null");
        }

        protected void encodeBOOL(boolean value) {
            out.append(value ? "true" : "false");
        }

        protected void encodeLONG(long value) {
            out.append(value);
        }

        protected void encodeDOUBLE(double value) {
            if (Double.isFinite(value)) {
                out.append(value);
            } else {
                out.append("null");
            }
        }

        static final char[] hex = "0123456789ABCDEF".toCharArray();

        protected void encodeSTRING(String value) {
            // Batching up some chars before calling out.append() is 20-60% faster depending on string size
            char[] buf = new char[128];
            char pos = 0;

            buf[pos++] = '"';

            for (char c : value.toCharArray()) {
                if(pos > buf.length - 8) {
                    out.append(buf, 0, pos);
                    pos = 0;
                }
                switch (c) {
                case '"':  buf[pos++] ='\\';buf[pos++] ='"'; break;
                case '\\': buf[pos++] ='\\';buf[pos++] ='\\'; break;
                case '\b': buf[pos++] ='\\';buf[pos++] ='b'; break;
                case '\f': buf[pos++] ='\\';buf[pos++] ='f'; break;
                case '\n': buf[pos++] ='\\';buf[pos++] ='n'; break;
                case '\r': buf[pos++] ='\\';buf[pos++] ='r'; break;
                case '\t': buf[pos++] ='\\';buf[pos++] ='t'; break;
                default:
                    if (c > 0x1f) {
                        buf[pos++] =c;
                    } else { // requires escaping according to RFC 4627
                        buf[pos++] ='\\'; buf[pos++] ='u';
                        buf[pos++] =hex[(c >> 12) & 0xf];
                        buf[pos++] =hex[(c >> 8) & 0xf];
                        buf[pos++] =hex[(c >> 4) & 0xf];
                        buf[pos++] =hex[c & 0xf];
                    }
                }
            }
            buf[pos++] ='"';

            out.append(buf, 0, pos);
        }

        protected void encodeDATA(byte[] value) {
            out.append('"');
            out.append("0x");
            for (int pos = 0; pos < value.length; pos++) {
                out.append(hex[(value[pos] >> 4) & 0xf]);
                out.append(hex[value[pos] & 0xf]);
            }
            out.append('"');
        }

        protected void encodeARRAY(Inspector inspector) {
            openScope("[");
            ArrayTraverser at = this;
            inspector.traverse(at);
            closeScope("]");
        }

        protected void encodeOBJECT(Inspector inspector) {
            openScope("{");
            ObjectTraverser ot = this;
            inspector.traverse(ot);
            closeScope("}");
        }

        private void openScope(String opener) {
            out.append(opener);
            level++;
            head = true;
        }

        private void closeScope(String closer) {
            level--;
            separate(false);
            out.append(closer);
        }

        private void encodeValue(Inspector inspector) {
            switch(inspector.type()) {
            case EMPTY:  encodeEMPTY();                      return;
            case BOOL:   encodeBOOL(inspector.asBool());     return;
            case LONG:   encodeLONG(inspector.asLong());     return;
            case DOUBLE: encodeDOUBLE(inspector.asDouble()); return;
            case STRING: encodeSTRING(inspector.asString()); return;
            case DATA:   encodeDATA(inspector.asData());     return;
            case ARRAY:  encodeARRAY(inspector);             return;
            case OBJECT: encodeOBJECT(inspector);            return;
            }
            assert false : "Should not be reached";
        }

        private void separate(boolean useComma) {
            if (!head && useComma) {
                out.append(',');
            } else {
                head = false;
            }
            if (!compact) {
                out.append("\n");
                for (int lvl = 0; lvl < level; lvl++) { out.append(" "); }
            }
        }

        public void entry(int idx, Inspector inspector) {
            separate(true);
            encodeValue(inspector);
        }

        public void field(String name, Inspector inspector)  {
            separate(true);
            encodeSTRING(name);
            out.append(':');
            if (!compact)
                out.append(' ');
            encodeValue(inspector);
        }

        /** Returns the target this is encoding values to */
        public StringBuilder target() { return out; }

    }

}
