/*
 Copyright (c) 2013-2016 Chukong Technologies Inc.
 Copyright (c) 2017-2020 Xiamen Yaji Software Co., Ltd.

 http://www.cocos.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated engine source code (the "Software"), a limited,
  worldwide, royalty-free, non-assignable, revocable and non-exclusive license
 to use Cocos Creator solely to develop games on your target platforms. You shall
  not use Cocos Creator software for developing other software or tools that's
  used for developing games. You are not granted to publish, distribute,
  sublicense, and/or sell copies of Cocos Creator.

 The software or tools in this License Agreement are licensed, not sold.
 Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

import { ccclass, type, serializable, editable } from 'cc.decorator';
import { Font } from './font';
import { SpriteFrame } from './sprite-frame';
import { cclegacy, js, warn } from '../../core';

export interface IConfig {
    [key: string]: any;
}

export class FontLetterDefinition {
    public u = 0;
    public v = 0;
    public w = 0;
    public h = 0;
    public offsetX = 0;
    public offsetY = 0;
    public textureID = 0;
    public valid = false;
    public xAdvance = 0;
}

export interface ILetterDefinition {
    [key: string]: FontLetterDefinition;
}

export class FontAtlas {
    public declare letterDefinitions;
    public declare texture;

    constructor (texture) {
        this.letterDefinitions = {};
        this.texture = texture;
    }

    public addLetterDefinitions (letter, letterDefinition) {
        this.letterDefinitions[letter] = letterDefinition;
    }

    public cloneLetterDefinition () {
        const copyLetterDefinitions: ILetterDefinition = {};
        for (const key of Object.keys(this.letterDefinitions)) {
            const value = new FontLetterDefinition();
            js.mixin(value, this.letterDefinitions[key]);
            copyLetterDefinitions[key] = value;
        }
        return copyLetterDefinitions;
    }

    public getTexture () {
        return this.texture;
    }

    public getLetter (key) {
        return this.letterDefinitions[key];
    }

    public getLetterDefinitionForChar (char, labelInfo?) {
        const key = char.charCodeAt(0);
        const hasKey = this.letterDefinitions.hasOwnProperty(key);
        let letter;
        if (hasKey) {
            letter = this.letterDefinitions[key];
        } else {
            letter = null;
        }
        return letter;
    }

    public clear () {
        this.letterDefinitions = {};
    }
}

/**
 * @en Class for BitmapFont handling.
 * @zh 位图字体资源类。
 */
@ccclass('cc.BitmapFont')
export class BitmapFont extends Font {
    @serializable
    @editable
    public fntDataStr = '';

    /**
     * @en [[SpriteFrame]] of the bitmap font
     * @zh 位图字体所使用的精灵。
     */
    @type(SpriteFrame)
    public spriteFrame: SpriteFrame | null = null;

    /**
     * @en The font size
     * @zh 文字尺寸。
     */
    @serializable
    @editable
    public fontSize = -1;

    /**
     * @en Font configuration
     * @zh 字体配置。
     */
    @serializable
    @editable
    public fntConfig: IConfig | null = null;

    public declare fontDefDictionary: FontAtlas;

    onLoaded () {
        const spriteFrame = this.spriteFrame;
        if (!this.fontDefDictionary && spriteFrame) {
            this.fontDefDictionary = new FontAtlas(spriteFrame.texture);
        }

        const fntConfig = this.fntConfig;
        if (!fntConfig) {
            warn('The fnt config is not exists!');
            return;
        }

        const fontDict = fntConfig.fontDefDictionary;
        for (const fontDef in fontDict) {
            const letter = new FontLetterDefinition();
            const rect = fontDict[fontDef].rect;
            letter.offsetX = fontDict[fontDef].xOffset;
            letter.offsetY = fontDict[fontDef].yOffset;
            letter.w = rect.width;
            letter.h = rect.height;
            letter.u = rect.x;
            letter.v = rect.y;
            // FIXME: only one texture supported for now
            letter.textureID = 0;
            letter.valid = true;
            letter.xAdvance = fontDict[fontDef].xAdvance;

            this.fontDefDictionary.addLetterDefinitions(fontDef, letter);
        }
    }
}

cclegacy.BitmapFont = BitmapFont;
