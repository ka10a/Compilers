#include "Linearizer.h"
#include <cassert>


namespace IRT
{
    Linearizer::Linearizer(): stackDepthCounter( 1, std::numeric_limits<int>::max() - 1 )
    {}

    std::unique_ptr<const IRStatement> Linearizer::CanonicalTree()
    {
        return std::move( prevStm );
    }

    void Linearizer::updateLastExp( const IRExpression* newLastExp )
    {
        prevExp = std::move( std::unique_ptr<const IRExpression>( newLastExp ) );
    }

    void Linearizer::updateLastExp( std::unique_ptr<const IRExpression> newLastExp )
    {
        prevExp = std::move( newLastExp );
    }

    void Linearizer::updateLastExpList( const IRExpList* newLastExpList )
    {
        prevExpList = std::move( std::unique_ptr<const IRExpList>( newLastExpList ) );
    }

    void Linearizer::updateLastExpList( std::unique_ptr<IRExpList> newLastExpList )
    {
        prevExpList = std::move( newLastExpList );
    }

    void Linearizer::updateLastStm( const IRStatement* newLastStm )
    {
        prevStm = std::move( std::unique_ptr<const IRStatement>( newLastStm ) );
    }

    void Linearizer::updateLastStm( std::unique_ptr<const IRStatement> newLastStm )
    {
        prevStm = std::move( newLastStm );
    }

    void Linearizer::visit( const ConstExpression* n )
    {
        ++stackDepthCounter.back();
        updateLastExp( std::make_unique<const ConstExpression>( n->value ) );
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const NameExpression* n )
    {
        ++stackDepthCounter.back();
        updateLastExp( std::make_unique<const NameExpression>( n->name ) );
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const TempExpression* n )
    {
        ++stackDepthCounter.back();
        updateLastExp( std::make_unique<const TempExpression>( n->name ) );
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const BinOpExpression* n )
    {
        ++stackDepthCounter.back();
        n->left->Accept( this );
        std::unique_ptr<const IRExpression> nLeft = std::move( prevExp );

        n->right->Accept( this );
        std::unique_ptr<const IRExpression> nRight = std::move( prevExp );

        updateLastExp(
                std::make_unique<const BinOpExpression>(
                        n->binop,
                        std::move(nLeft),
                        std::move(nRight)
                )
        );
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const MemExpression* n )
    {
        ++stackDepthCounter.back();
        n->expr->Accept( this );
        std::unique_ptr<const IRExpression> addressExp = std::move( prevExp );

        updateLastExp(
                std::make_unique<const MemExpression>( addressExp.release() )
        );
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const CallExpression* n )
    {
        ++stackDepthCounter.back();
        n->func->Accept( this );
        std::unique_ptr<const IRExpression> functionExp = std::move( prevExp );

        n->args->Accept(this);
        std::unique_ptr<const IRExpList> argumentsList = std::move( prevExpList );
        updateLastExp(
                std::make_unique<const CallExpression> (
                    std::move(functionExp),
                    std::move(argumentsList)
                )
        );
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const ESeqExpression* n )
    {
        assert(false);
    }

    void Linearizer::visit( const ExpStatement* n )
    {
        ++stackDepthCounter.back();
        n->exp->Accept( this );
        std::unique_ptr<const IRExpression> exp = std::move( prevExp );

        std::unique_ptr<const IRStatement> res(
            std::move(
                std::make_unique<const ExpStatement>(std::move(exp))));
        saveCreatedStm(std::move(res));
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const CJumpStatement* n )
    {
        ++stackDepthCounter.back();
        n->left->Accept( this );
        std::unique_ptr<const IRExpression> nLeft = std::move( prevExp );

        n->right->Accept( this );
        std::unique_ptr<const IRExpression> nRight = std::move( prevExp );

        std::unique_ptr<const IRStatement> res(
            std::move(
                std::make_unique<const CJumpStatement>(
                    n->rel,
                    nLeft.release(),
                    nRight.release(),
                    n->if_left,
                    n->if_right
                )
            )
        );
        saveCreatedStm(std::move(res));
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const JumpStatement* n )
    {
        ++stackDepthCounter.back();
        std::unique_ptr<const IRStatement> res(
            std::move(
                std::make_unique<const JumpStatement>(n->label)
            )
        );
        saveCreatedStm(std::move(res));
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const LabelStatement* n )
    {
        ++stackDepthCounter.back();
        std::unique_ptr<const IRStatement> res(
            std::move(
                std::make_unique<const LabelStatement>(n->GetLabel())
            )
        );
        saveCreatedStm(std::move(res));
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const MoveStatement* n )
    {
        ++stackDepthCounter.back();
        n->dst->Accept( this );
        std::unique_ptr<const IRExpression> destination = std::move( prevExp );

        n->src->Accept( this );
        std::unique_ptr<const IRExpression> source = std::move( prevExp );

        std::unique_ptr<const IRStatement> res(
            std::move(
                std::make_unique<const MoveStatement>(
                    destination.release(),
                    source.release()
                )
            )
        );
        saveCreatedStm(std::move(res));
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const SeqStatement* n )
    {
        ++stackDepthCounter.back();
        if (stackDepthCounter.back() > 1) {
            stackOfSeqChildren.push_back(std::vector<std::unique_ptr<const IRStatement>>());
        }
        stackDepthCounter.push_back(0);

        n->left->Accept( this );
        n->right->Accept( this );

        stackDepthCounter.pop_back();
        if (stackDepthCounter.back() > 1) {
            IRStatementList* stm_list = new IRStatementList();
            for (auto& it: stackOfSeqChildren.back()) {
                stm_list->Add(std::move(it));
            }
            updateLastStm(stm_list);
            stackOfSeqChildren.pop_back();
        }
        --stackDepthCounter.back();
    }

    void Linearizer::visit( const IRExpList* Explist )
    {
        auto newList = std::make_unique<IRExpList>();
        const auto& arguments = Explist->list;
        for( const auto& arg : arguments ) {
            arg->Accept( this );
            newList->Add( std::move( prevExp ) );
        }
        updateLastExpList( std::move( newList ) );
    }

    void Linearizer::visit(const IRStatementList* list) {
        assert(false);
    }

    void Linearizer::saveCreatedStm(std::unique_ptr<const IRStatement> result) {
        if (stackDepthCounter.back() == 1) {
            stackOfSeqChildren.back().push_back(std::move(result));
        } else {
            updateLastStm(std::move(result));
        }
    }

}