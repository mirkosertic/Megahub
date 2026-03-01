import { describe, it, expect, vi } from 'vitest'
import { definition } from '../../src/components/blockly/mh_fastled_show.js'

function makeBlock(fields = {}, values = {}, statements = {}, extra = {}) {
    return {
        getFieldValue: (name) => fields[name] ?? null,
        id: extra.id ?? 'test-block-id',
        outputConnection: extra.outputConnection ?? null,
        ...extra
    }
}

function makeGenerator(values = {}, statements = {}) {
    return {
        valueToCode: (block, name, _order) => values[name] ?? '',
        statementToCode: (block, name) => statements[name] ?? ''
    }
}

describe('mh_fastled_show block definition', () => {
    it('has category "FastLED"', () => {
        expect(definition.category).toBe('FastLED')
    })

    it('has type "mh_fastled_show"', () => {
        expect(definition.blockdefinition.type).toBe('mh_fastled_show')
    })

    it('has previousStatement and nextStatement', () => {
        expect(definition.blockdefinition.previousStatement).toBe(true)
        expect(definition.blockdefinition.nextStatement).toBe(true)
    })
})

describe('mh_fastled_show generator', () => {
    it('returns "fastled.show()\\n" regardless of inputs', () => {
        const block = makeBlock()
        const gen = makeGenerator()
        const code = definition.generator(block, gen)
        expect(code).toBe('fastled.show()\n')
    })

    it('always returns the same output', () => {
        const block = makeBlock({ SOME_FIELD: 'some_value' })
        const gen = makeGenerator({ SOME_INPUT: 'some_code' })
        const code = definition.generator(block, gen)
        expect(code).toBe('fastled.show()\n')
    })
})
