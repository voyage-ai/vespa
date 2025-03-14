// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.indexinglanguage.expressions;

import com.yahoo.document.DocumentType;
import com.yahoo.document.Field;
import com.yahoo.vespa.indexinglanguage.ExpressionConverter;
import com.yahoo.vespa.objects.ObjectOperation;
import com.yahoo.vespa.objects.ObjectPredicate;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Objects;

/**
 * @author Simon Thoresen Hult
 */
public abstract class ExpressionList<T extends Expression> extends CompositeExpression implements Iterable<T> {

    private final List<T> expressions = new ArrayList<T>();

    protected ExpressionList(Iterable<? extends T> expressions) {
        for (T exp : expressions)
            this.expressions.add(exp);
    }

    @Override
    public boolean requiresInput() { return !expressions.isEmpty() && expressions.get(0).requiresInput(); }

    public List<T> expressions() { return expressions; }

    protected List<Expression> convertChildList(ExpressionConverter converter) {
        return asList().stream().map(converter::convert).filter(Objects::nonNull).toList();
    }

    @Override
    public void setStatementOutput(DocumentType documentType, Field field) {
        for (Expression expression : expressions)
            expression.setStatementOutput(documentType, field);
    }

    public int size() {
        return expressions.size();
    }

    public T get(int idx) {
        return expressions.get(idx);
    }

    public boolean isEmpty() {
        return expressions.isEmpty();
    }

    public List<T> asList() {
        return Collections.unmodifiableList(expressions);
    }

    @Override
    public Iterator<T> iterator() {
        return expressions.iterator();
    }

    @Override
    @SuppressWarnings("rawtypes")
    public boolean equals(Object obj) {
        if (!(obj instanceof ExpressionList rhs)) return false;
        if (!expressions.equals(rhs.expressions)) return false;
        return true;
    }

    @Override
    public int hashCode() {
        return getClass().hashCode() + expressions.hashCode();
    }

    @Override
    public void selectMembers(ObjectPredicate predicate, ObjectOperation operation) {
        for (T exp : expressions) {
            exp.select(predicate, operation);
        }
    }

}
