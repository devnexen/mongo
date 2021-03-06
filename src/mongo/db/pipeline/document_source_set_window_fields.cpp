/**
 *    Copyright (C) 2020-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/db/pipeline/document_source_set_window_fields.h"
#include "mongo/db/pipeline/document_source_set_window_fields_gen.h"
#include "mongo/db/pipeline/lite_parsed_document_source.h"
#include "mongo/db/query/query_feature_flags_gen.h"

namespace mongo {

REGISTER_DOCUMENT_SOURCE_CONDITIONALLY(
    setWindowFields,
    LiteParsedDocumentSourceDefault::parse,
    DocumentSourceSetWindowFields::createFromBson,
    boost::none,
    ::mongo::feature_flags::gFeatureFlagWindowFunctions.isEnabledAndIgnoreFCV());

Value DocumentSourceSetWindowFields::serialize(
    boost::optional<ExplainOptions::Verbosity> explain) const {
    MutableDocument spec;
    spec[SetWindowFieldsSpec::kPartitionByFieldName] =
        _partitionBy ? (*_partitionBy)->serialize(false) : Value();
    spec[SetWindowFieldsSpec::kSortByFieldName] = _sortBy ? Value(*_sortBy) : Value();
    spec[SetWindowFieldsSpec::kOutputFieldName] = Value(_fields);
    return Value(DOC(kStageName << spec.freeze()));
}

boost::intrusive_ptr<DocumentSource> DocumentSourceSetWindowFields::createFromBson(
    BSONElement elem, const boost::intrusive_ptr<ExpressionContext>& expCtx) {
    uassert(ErrorCodes::FailedToParse,
            str::stream() << "the " << kStageName
                          << " stage specification must be an object, found "
                          << typeName(elem.type()),
            elem.type() == BSONType::Object);

    auto spec =
        SetWindowFieldsSpec::parse(IDLParserErrorContext(kStageName), elem.embeddedObject());
    auto partitionBy = [&]() -> boost::optional<boost::intrusive_ptr<Expression>> {
        if (auto partitionBy = spec.getPartitionBy())
            return Expression::parseOperand(
                expCtx.get(), partitionBy->getElement(), expCtx->variablesParseState);
        else
            return boost::none;
    }();
    return make_intrusive<DocumentSourceSetWindowFields>(
        expCtx, partitionBy, spec.getSortBy(), spec.getOutput());
}

DocumentSource::GetNextResult DocumentSourceSetWindowFields::doGetNext() {
    return GetNextResult::makeEOF();
}

}  // namespace mongo
