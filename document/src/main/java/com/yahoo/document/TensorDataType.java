// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.document;

import com.yahoo.document.datatypes.FieldValue;
import com.yahoo.document.datatypes.TensorFieldValue;
import com.yahoo.tensor.TensorType;
import com.yahoo.vespa.objects.Ids;

import java.util.Objects;

/**
 * A DataType containing a tensor type
 *
 * @author bratseth
 */
public class TensorDataType extends DataType {

    // The global class identifier shared with C++.
    public static int classId = registerClass(Ids.document + 59, TensorDataType.class);

    private static final TensorDataType anyTensorDataType = new TensorDataType(null);

    private final TensorType tensorType;

    public TensorDataType(TensorType tensorType) {
        super(tensorType == null ? "tensor" : tensorType.toString(), DataType.tensorDataTypeCode);
        this.tensorType = tensorType;
    }

    @Override
    public TensorDataType clone() {
        return (TensorDataType)super.clone();
    }

    @Override
    public FieldValue createFieldValue() {
        return new TensorFieldValue(tensorType);
    }

    @Override
    public Class<? extends TensorFieldValue> getValueClass() {
        return TensorFieldValue.class;
    }

    @Override
    public boolean isValueCompatible(FieldValue value) {
        if (value == null) return false;
        if (tensorType == null) return true; // any
        if ( ! TensorFieldValue.class.isAssignableFrom(value.getClass())) return false;
        TensorFieldValue tensorValue = (TensorFieldValue)value;
        return tensorValue.getDataType().getTensorType().isConvertibleTo(tensorType);
    }

    /** Returns the type of the tensor this field can hold */
    public TensorType getTensorType() { return tensorType; }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        if (!super.equals(o)) return false;
        TensorDataType that = (TensorDataType) o;
        return Objects.equals(tensorType, that.tensorType);
    }

    @Override
    public int hashCode() {
        return Objects.hash(super.hashCode(), tensorType);
    }

    /** Returns the tensor data type representing any tensor. */
    public static TensorDataType any() { return anyTensorDataType; }

}
